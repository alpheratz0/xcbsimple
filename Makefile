LDLIBS = -lxcb -lxcb-image
LDFLAGS = -s ${LDLIBS}
INCS = -I/usr/include
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Os ${INCS}
CC = cc

SRC = src/xcbsimple.c \
	  src/base/bitmap.c \
	  src/util/debug.c \
	  src/x11/window.c

OBJ = ${SRC:.c=.o}

all: xcbsimple

${OBJ}:	src/base/bitmap.h \
		src/util/debug.h \
		src/util/numdef.h \
		src/x11/window.h \
		src/x11/keys.h

xcbsimple: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f xcbsimple ${OBJ}

.PHONY: all clean
