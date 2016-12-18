include config.mk

SRC = dwmstat.c
OBJ = ${SRC:.c=.o}

all: dwmstat

.c.o:
	@${CC} -c ${CFLAGS} $<

${OBJ}:
	config.mk

dwmstatus: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f dwmstat ${OBJ}

install: all
	@mkdir -p "${DESTDIR}${PREFIX}"/bin/
	@cp -f dwmstat "${DESTDIR}${PREFIX}"/bin/
	@chmod 755 "${DESTDIR}${PREFIX}"/bin/dwmstat

uninstall:
	@rm -f "${DESTDIR}${PREFIX}"/bin/dwmstat

.PHONY: all clean install uninstall
