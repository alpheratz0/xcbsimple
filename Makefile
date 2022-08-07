.POSIX:
.PHONY: all clean

CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Os
LDLIBS  = -lxcb -lxcb-image
LDFLAGS = -s

all: xcbsimple

xcbsimple: xcbsimple.o
	$(CC) $(LDFLAGS) -o xcbsimple xcbsimple.o $(LDLIBS)

clean:
	rm -f xcbsimple xcbsimple.o
