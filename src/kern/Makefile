# ==============================================================================
# Makefile do kernel (mkrn) - amd64/bios
#
# ainda roda em modo protegido de 32 bits (e o que o multiboot garante);
# a transicao pra modo longo fica pra uma etapa futura. por isso o build
# e -m32 e nao usa o prog.mk (kernel nao linka contra libc).
#
# -nostdinc + -idirafter no diretorio freestanding do proprio compilador:
# nada do sistema (glibc, musl, o que for) fica alcancavel, so os headers
# daqui (sys/, amd64/include/) e o que o gcc garante como freestanding de
# verdade (stdarg.h - o resto, tipos, e nosso, ver sys/types.h).
# ==============================================================================

PROG=		mkrn

CC?=		cc
LD?=		ld

GCC_FREESTANDING_INC != ${CC} -m32 -print-file-name=include

CFLAGS=		-m32 -std=c99 -ffreestanding -fno-pie -fno-stack-protector \
		-fno-builtin -Wall -Wextra -O2 -g \
		-nostdinc -idirafter ${GCC_FREESTANDING_INC} \
		-MMD -MP
LDFLAGS=	-m elf_i386 -nostdlib -T amd64/kern.ld

MISRCS=		main.c subr_log.c subr_prf.c subr_panic.c subr_libkern.c subr_pmm.c \
		subr_vmm.c subr_kmalloc.c subr_font.c subr_thread.c
MDCSRCS=	amd64/vga.c amd64/fb.c amd64/cons.c amd64/tsc.c amd64/gdt.c amd64/idt.c \
		amd64/trap.c amd64/pic.c amd64/irq.c amd64/pit.c amd64/reboot.c \
		amd64/pmm_boot.c amd64/pmap.c
MDSSRCS=	amd64/boot.S amd64/gdt_flush.S amd64/idt_stubs.S amd64/switch.S

OBJS=		${MISRCS:.c=.o} ${MDCSRCS:.c=.o} ${MDSSRCS:.S=.o}
DEPS=		${OBJS:.o=.d}

all: ${PROG}

# kern.ld e gerado a partir de kern.lds pelo preprocessador de c, so pra
# nao duplicar KERNBASE/KERNPHYS (definidos em machine/pmap.h) numa
# terceira linguagem - ver o comentario no topo de amd64/kern.lds
amd64/kern.ld: amd64/kern.lds amd64/include/machine/kernbase.h
	${CC} -E -x c -P -nostdinc -idirafter ${GCC_FREESTANDING_INC} $< -o $@

.c.o:
	${CC} ${CFLAGS} -c $< -o $@

.S.o:
	${CC} ${CFLAGS} -c $< -o $@

${PROG}: ${OBJS} amd64/kern.ld
	${LD} ${LDFLAGS} ${OBJS} -o $@

clean:
	rm -f ${PROG} ${OBJS} ${DEPS} amd64/kern.ld

-include ${DEPS}

.PHONY: all clean
