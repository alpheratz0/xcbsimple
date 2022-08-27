.POSIX:
.PHONY: all clean

include config.mk

all: xcbsimple

xcbsimple: xcbsimple.o
	$(CC) $(LDFLAGS) -o xcbsimple xcbsimple.o $(LDLIBS)

clean:
	rm -f xcbsimple xcbsimple.o
