PROG   = dwmstat
NOMAN  = noman
BINDIR = /usr/local/bin

WARNINGS = Yes
CFLAGS  += -std=c99 -pedantic -Wextra -Werror
CFLAGS  += -Wno-error=variadic-macros
CFLAGS  += -I/usr/X11R6/include
LDFLAGS += -L/usr/X11R6/lib
LDADD   += -lX11

.include <bsd.prog.mk>

${OBJS}: config.h

config.h:
	@cp config.def.h $@
