ifeq ($(CC),cc)
CC = gcc
endif
AR = ar rcv
ifeq ($(windir),)
EXE =
RM = rm -f
else
EXE = .exe
RM = del
endif

CFLAGS = -ffunction-sections -O3

ifneq (,$(findstring darwin,$(CROSS_COMPILE)))
    UNAME_S := Darwin
else
    UNAME_S := $(shell uname -s)
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += -Wl,-dead_strip
else
    LDFLAGS += -Wl,--gc-sections -s
endif

all:dhtbsign$(EXE)

static:
	$(MAKE) LDFLAGS="$(LDFLAGS) -static"

libmincrypt.a:
	$(MAKE) -C libmincrypt

dhtbsign$(EXE):dhtbsign.o libmincrypt.a
	$(CROSS_COMPILE)$(CC) -o $@ $^ -L. -lmincrypt $(LDFLAGS)

dhtbsign.o:dhtbsign.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< -I. -Werror

clean:
	$(RM) dhtbsign
	$(RM) *.a *.~ *.exe *.o
	$(MAKE) -C libmincrypt clean

