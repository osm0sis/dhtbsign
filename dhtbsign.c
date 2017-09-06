#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include "mincrypt/sha256.h"

const char DHTB_MAGIC[] =       {0x44, 0x48, 0x54, 0x42, 0x01, 0x00, 0x00, 0x00};
#define DHTB_HEADER_SZ          512
#define SHA256_OFFSET           8
#define PAYLOAD_SZ_OFFSET       48
#define BOOT_MAGIC              "ANDROID!"
#define BOOT_MAGIC_SZ           8
#define SEANDROID_MAGIC         "SEANDROIDENFORCE"
#define SEANDROID_MAGIC_SZ      16
const char PAYLOAD_PADDING[] =  {0xFF, 0xFF, 0xFF, 0xFF};
#define TRAILING_PADDING_SZ     12

int usage()
{
    printf("usage: dhtbsign\n");
    printf("\t-i|--input boot.img\n");
    printf("\t-o|--output boot_t280.img\n");
    return 0;
}

int main (int argc, char** argv)
{
    char* iname = NULL;
    char* oname = NULL;
    char tmp[4096];

    argc--;
    argv++;
    while(argc > 0) {
        char *arg = argv[0];
        char *val = argv[1];
        argc -= 2;
        argv += 2;
        if (!strcmp(arg, "--input") || !strcmp(arg, "-i"))
            iname = val;
        else if (!strcmp(arg, "--output") || !strcmp(arg, "-o"))
            oname = val;
        else
            return usage();
    }

    if ((iname == NULL) || (oname == NULL))
        return usage();

    FILE *ifile = fopen(iname, "rb");
    if (ifile == NULL) {
        printf("Error opening input file %s!\n", iname);
        return usage();
    }

    //Check for DHTB magic
    fseek(ifile, 0, SEEK_SET);
    if(fread(tmp, sizeof(DHTB_MAGIC), 1, ifile)){};
    if (memcmp(tmp, DHTB_MAGIC, sizeof(DHTB_MAGIC)) == 0) {
        printf("DHTB header magic already present!\n");
        fclose(ifile);
        return 1;
    }

    //Check for ANDROID! magic
    fseek(ifile, 0, SEEK_SET);
    if(fread(tmp, BOOT_MAGIC_SZ, 1, ifile)){};
    if (memcmp(tmp, BOOT_MAGIC, BOOT_MAGIC_SZ) != 0) {
        printf("Android boot magic not found!\n");
        fclose(ifile);
        return 1;
    }

    //Read boot.img filesize
    fseek(ifile, 0, SEEK_END);
    long isize = ftell(ifile);

    FILE *ofile = fopen(oname, "wb");
    if (ofile == NULL) {
        printf("Error opening output file %s!\n", oname);
        fclose(ifile);
        return usage();
    }

    //Check if SEANDROIDENFORCE footer and/or 0xFFFFFFFF padding are needed
    printf("Input filesize: %d\n", isize);
    int seeklimit = 65536;
    int need_seandroid = 1;
    int need_plpadding = 1;
    for (int i = isize - seeklimit; i <= isize; i++) {
        fseek(ifile, i, SEEK_SET);
        if(fread(tmp, SEANDROID_MAGIC_SZ, 1, ifile)){};
        if (memcmp(tmp, SEANDROID_MAGIC, SEANDROID_MAGIC_SZ) == 0) {
            printf("SEAndroid footer at: %d\n", i);
            need_seandroid = 0;
            i = i + SEANDROID_MAGIC_SZ;
            fseek(ifile, i, SEEK_SET);
            if(fread(tmp, sizeof(PAYLOAD_PADDING), 1, ifile)){};
            if (memcmp(tmp, PAYLOAD_PADDING, sizeof(PAYLOAD_PADDING)) == 0) {
                printf("Payload padding at: %d\n", i);
                need_plpadding = 0;
                i = i + sizeof(PAYLOAD_PADDING);
            }
            isize = i;
            break;
        }
    }
    printf("Input read size: %d\n", isize);

    //Allocate header + boot.img size + trailing padding
    unsigned char *bootimg = malloc(DHTB_HEADER_SZ + isize);
    if (bootimg == NULL)
        printf("Error allocating %ld bytes for output file!", (DHTB_HEADER_SZ + isize + 12));
    fseek(ifile, 0, SEEK_SET);
    if(fread((DHTB_HEADER_SZ + bootimg), isize, 1, ifile)){};
    fclose(ifile);

    //Append SEANDROIDENFORCE footer and/or 0xFFFFFFFF padding if needed
    if (need_seandroid == 1) {
        printf("Adding SEANDROIDENFORCE footer...\n");
        memcpy(bootimg + (DHTB_HEADER_SZ + isize), SEANDROID_MAGIC, SEANDROID_MAGIC_SZ);
        isize = isize + SEANDROID_MAGIC_SZ;
    }
    if (need_plpadding == 1) {
        printf("Adding payload padding...\n");
        memcpy(bootimg + (DHTB_HEADER_SZ + isize), PAYLOAD_PADDING, sizeof(PAYLOAD_PADDING));
        isize = isize + sizeof(PAYLOAD_PADDING);
    }

    //Zero DHTB header
    memset(bootimg, 0, DHTB_HEADER_SZ);

    //Calculate payload size
    printf("Payload size: %d\n", isize);
    uint32_t psize = isize;

    //Compute SHA256 of payload
    SHA256_CTX ctx256;
    SHA256_init(&ctx256);
    SHA256_update(&ctx256, (DHTB_HEADER_SZ + bootimg), psize);
    const uint8_t* sha256 = SHA256_final(&ctx256);

    //Copy MAGIC
    memcpy(bootimg, DHTB_MAGIC, sizeof(DHTB_MAGIC));

    //Copy SHA256
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++)
        bootimg[SHA256_OFFSET+i] = sha256[i];

    //Copy payload size as LE
    bootimg[PAYLOAD_SZ_OFFSET+0] = (psize >>  0) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+1] = (psize >>  8) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+2] = (psize >> 16) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+3] = (psize >> 24) & 0xFF;

    //Write output file with additional padding, close/free
    printf("Writing DHTB header + payload...\n");
    fwrite(bootimg, (DHTB_HEADER_SZ + isize + TRAILING_PADDING_SZ), 1, ofile);
    fclose(ofile);
    free(bootimg);
 }
