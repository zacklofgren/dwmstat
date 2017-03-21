BINDIR = /usr/local/bin
NOMAN  = noman
PROG   = dwmstat

CC       = egcc
CFLAGS  += -std=c99 -pedantic -Wextra -Werror
CFLAGS  += -Wno-error=variadic-macros
CFLAGS  += -I/usr/X11R6/include
LDFLAGS += -L/usr/X11R6/lib
LDADD   += -lX11
WARNINGS = Yes

.include <bsd.prog.mk>

${OBJS}: config.h

config.h:
	@cp config.def.h $@
