#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <arpa/inet.h>
#include "mincrypt/sha256.h"

const char T280_MAGIC[] =       {0x44, 0x48, 0x54, 0x42,0x01, 0x00, 0x00, 0x00};
#define T280_HEADER_SZ          512
#define T280_MAGIC_OFFSET       0
#define SHA256_OFFSET           8
#define PAYLOAD_SZ_OFFSET       48
#define SEANDROID_HEADER_SZ     680
#define SEANDROID_MAGIC_SZ      20

// Assumption is mkbootimg was used with --signature option to add on the required SEANDROID MAGIC (20 bytes) for sha256 calculation
// Use "tools/seandroid_t280.img" or take a stock boot.img and run " tail -c 680 boot.img > seandroid_t280.img"

int usage()
{
    printf("usage: mkT280bootimg\n");
    printf("\t-i|--input boot.img\n");
    printf("\t-o|--output boot_t280.img\n");
    return 0;
}

int main (int argc, char** argv)
{
    char* iname = NULL;
    char* oname = NULL;

    argc--;
    argv++;
    while(argc > 0)
    {
        char *arg = argv[0];
        char *val = argv[1];
        argc -= 2;
        argv += 2;
        if (!strcmp(arg, "--input") || !strcmp(arg, "-i"))
        {
            iname = val;
        }
        else if (!strcmp(arg, "--output") || !strcmp(arg, "-o"))
        {
            oname = val;
        }
        else
        {
            return usage();
        }
    }

    if ((iname == NULL) || (oname == NULL)) {
        return usage();
    }

    FILE *ifile = fopen(iname, "rb");
    if( ifile == NULL )  {
        printf("Error opening input file %s!\n", iname);
        return usage();
    }

    FILE *ofile = fopen(oname, "wb");
    if( ofile == NULL )  {
        printf("Error opening output file %s!\n", oname);
        fclose(ifile);
        return usage();
    }

    //Read boot.img filesize
    fseek(ifile, 0, SEEK_END);
    long isize = ftell(ifile);
    fseek(ifile, 0, SEEK_SET);
    if (isize <= SEANDROID_HEADER_SZ)
    {
        printf("Input file size <= SEANDROID Header for T280 (%d bytes): %ld\n", SEANDROID_HEADER_SZ, isize);
        fclose(ifile);
        fclose(ofile);
    }

    //Allocate header + boot.img size
    unsigned char *bootimg = malloc(isize + T280_HEADER_SZ);
    if (bootimg == NULL)
    {
        printf("Error allocating %ld bytes for output file!", (isize + T280_HEADER_SZ) );
    }
    fread( (bootimg + T280_HEADER_SZ), isize, 1, ifile);
    fclose(ifile);

    //Zero T280 HEADER
    memset(bootimg, 0, T280_HEADER_SZ);

    //Calculate Payload Size
    uint32_t psize = (isize - (SEANDROID_HEADER_SZ - SEANDROID_MAGIC_SZ));

    //Compute SHA256 of payload
    SHA256_CTX ctx256;
    SHA256_init(&ctx256);
    SHA256_update(&ctx256, (bootimg + T280_HEADER_SZ), psize);
    const uint8_t* sha256 = SHA256_final(&ctx256);

    //Copy MAGIC
    memcpy((bootimg + T280_MAGIC_OFFSET), T280_MAGIC, sizeof(T280_MAGIC));

    //Copy SHA256
    for(int i = 0; i < SHA256_DIGEST_SIZE; i++)
    {
        bootimg[SHA256_OFFSET+i] = sha256[i];
    }

    //Copy Length
    bootimg[PAYLOAD_SZ_OFFSET+0] = (psize >>  0) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+1] = (psize >>  8) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+2] = (psize >> 16) & 0xFF;
    bootimg[PAYLOAD_SZ_OFFSET+3] = (psize >> 24) & 0xFF;

    //Write/Close/Free
    fwrite(bootimg, (isize + T280_HEADER_SZ), 1, ofile);
    fclose(ofile);
    free(bootimg);
 }
