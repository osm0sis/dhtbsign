ifeq ($(CC),cc)
CC = gcc
endif
AR = ar rc
ifeq ($(windir),)
EXT =
RM = rm -f
CP = cp
else
EXT = .exe
RM = del
CP = copy /y
endif

CFLAGS += -ffunction-sections -O3

INC = -I..

EXT = .a
LIB = libmincrypt$(EXT)
LIB_OBJS = dsa_sig.o p256.o p256_ec.o p256_ecdsa.o rsa.o sha.o sha256.o

all:$(LIB)

$(LIB):$(LIB_OBJS)
	$(CROSS_COMPILE)$(AR) $@ $^
	$(CP) $@ ..

%.o:%.c
	$(CROSS_COMPILE)$(CC) -o $@ $(CFLAGS) -c $< $(INC) -Werror

clean:
	$(RM) $(LIB_OBJS) $(LIB)

