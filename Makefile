.POSIX:
.PHONY: all clean

include config.mk

all: xcbsimple

xcbsimple: xcbsimple.o
	$(CC) $(LDFLAGS) -o xcbsimple xcbsimple.o

clean:
	rm -f xcbsimple xcbsimple.o
