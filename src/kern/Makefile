# ==============================================================================
# Makefile do kernel (mkrn) - amd64/bios
#
# ainda roda em modo protegido de 32 bits (e o que o multiboot garante);
# a transicao pra modo longo fica pra uma etapa futura. por isso o build
# e -m32 e nao usa o prog.mk (kernel nao linka contra libc).
# ==============================================================================

PROG=		mkrn

CC?=		cc
LD?=		ld

CFLAGS=		-m32 -std=c99 -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin -Wall -Wextra -O2 -g -Isys -Iamd64/include
LDFLAGS=	-m elf_i386 -nostdlib -T amd64/kern.ld

MISRCS=		main.c subr_log.c subr_prf.c subr_panic.c subr_libkern.c
MDCSRCS=	amd64/vga.c amd64/tsc.c amd64/gdt.c amd64/idt.c amd64/trap.c \
		amd64/pic.c amd64/irq.c amd64/pit.c amd64/reboot.c
MDSSRCS=	amd64/boot.S amd64/gdt_flush.S amd64/idt_stubs.S

OBJS=		${MISRCS:.c=.o} ${MDCSRCS:.c=.o} ${MDSSRCS:.S=.o}

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

.S.o:
	${CC} ${CFLAGS} -c $< -o $@

${PROG}: ${OBJS}
	${LD} ${LDFLAGS} ${OBJS} -o $@

clean:
	rm -f ${PROG} ${OBJS}

.PHONY: all clean
