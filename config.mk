PREFIX = /usr/local

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

INCS = -I${X11INC}
LIBS = -L${X11LIB} -lX11

CFLAGS  = -std=c99 -pedantic -Wall -Werror -Wno-error=variadic-macros -Wno-error=unused-function -Os ${INCS}
LDFLAGS = -s ${LIBS}

CC = cc
