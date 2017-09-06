CC = gcc
AR = ar rcv
ifeq ($(windir),)
EXE =
RM = rm -f
else
EXE = .exe
RM = del
endif

CFLAGS = -ffunction-sections -O3

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -Wl,-dead_strip
else
    LDFLAGS += -Wl,--gc-sections
endif

all:libmincrypt.a dhtbsign$(EXE)

static:
	make LDFLAGS="$(LDGLAGS) -static"

libmincrypt.a:
	make -C libmincrypt

dhtbsign$(EXE):dhtbsign.o
	$(CROSS_COMPILE)$(CC) -o $@ $^ -L. -lmincrypt $(LDFLAGS) -s

dhtbsign.o:dhtbsign.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -I. -Werror

clean:
	$(RM) dhtbsign dhtbsign.o libmincrypt.a
	make -C libmincrypt clean

