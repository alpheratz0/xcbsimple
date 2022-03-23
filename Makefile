LDLIBS = -lxcb -lxcb-image
LDFLAGS = -s ${LDLIBS}
INCS = -I/usr/include
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os ${INCS}
CC = cc

SRC = src/xcbsimple.c \
	  src/bitmap.c \
	  src/window.c \
	  src/debug.c

OBJ = ${SRC:.c=.o}

all: xcbsimple

${OBJ}:	src/bitmap.h \
		src/window.h \
		src/debug.h \
		src/numdef.h \
		src/keys.h

xcbsimple: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f xcbsimple ${OBJ}

.PHONY: all clean
