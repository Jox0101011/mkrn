# ==============================================================================
# prog.mk - template simples
# ==============================================================================

# 1. configuracoes padroes
CC?=            cc
CFLAGS?=        -O2 -pipe
PREFIX?=        /usr/local
BINDIR?=        ${PREFIX}/bin

# 2. deriva object files (.o) de source files (.c)
ifndef SRCS
OBJS=   ${PROG}.o
else
OBJS=   ${SRCS:.c=.o}
endif

# 3. targets principais
all: ${PROG}

# regra implicita: como transformar um .c em .o
.c.o:
	${CC} ${CFLAGS} -c $< -o $@

# regra de link: construir binario final
${PROG}: ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJS} ${LDADD} -o $@

# regra de limpeza: limpa lixo de build
clean:
	rm -f ${PROG} ${OBJS}

# regra de instalacao: instala binarios
install: all
	mkdir -p ${DESTDIR}${BINDIR}
	install -m 755 ${PROG} ${DESTDIR}${BINDIR}/${PROG}

.PHONY: all clean install
