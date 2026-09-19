# prog.mk
# 
# [USAGE]-~~--~~--~~~--~~~~--~~~-~~~--|
# |make -f prog.mk          |
# |make -f prog.mk all      |
# |make -f prog.mk clean    |
# |make -f prog.mk install  | 
# |make -f prog.mk .c.o     |
# |___________________________________|
#
# *-----------------------------*
# .PHONY -> all clean install
# *-----------------------------*

CC ?=            cc
CFLAGS ?=        -Os -pipe
PREFIX ?=        /usr/local
BINDIR ?=        ${PREFIX}/bin

ifndef SRCS
OBJS=   ${PROG}.o
else
OBJS=   ${SRCS:.c=.o}
endif

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

${PROG}: ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJS} ${LDADD} -o $@

clean:
	rm -f ${PROG} ${OBJS}

install: all
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 755 ${PROG} ${DESTDIR}${BINDIR}/${PROG}

.PHONY: all clean install
