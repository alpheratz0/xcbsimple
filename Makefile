CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -Os
LDLIBS  = -lxcb -lxcb-image
LDFLAGS = -s ${LDLIBS}

all: xcbsimple

.c.o:
	${CC} -c ${CFLAGS} $<

xcbsimple: xcbsimple.o
	${CC} -o $@ $< ${LDFLAGS}

clean:
	rm -f xcbsimple xcbsimple.o

.PHONY: all clean
