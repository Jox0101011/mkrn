/*
 * trap.c - trata as excecoes da cpu (vetores 0-31)
 *
 * ainda nao temos processos/sinais, entao "tratar" aqui significa
 * decodificar o que a excecao especifica da de informacao extra
 * (selector invalido, endereco de pagefault, etc) e so depois
 * chamar panic() - matar so a thread que causou o problema fica
 * pra quando existir essa nocao de thread.
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/cpufunc.h"
#include "include/machine/idt.h"
#include "include/machine/pic.h"
#include "../sys/log.h"
#include "../sys/panic.h"

#define T_DE	0	/* divide error */
#define T_UD	6	/* invalid opcode */
#define T_DF	8	/* double fault */
#define T_TS	10	/* invalid tss */
#define T_NP	11	/* segment not present */
#define T_SS	12	/* stack-segment fault */
#define T_GP	13	/* general protection */
#define T_PF	14	/* page fault */

/* nomes das excecoes da cpu, vetores 0-31 (intel sdm vol.3, cap.6) */
static const char *const trap_name[NEXC] = {
	"divide error",
	"debug",
	"nmi",
	"breakpoint",
	"overflow",
	"bound range exceeded",
	"invalid opcode",
	"device not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid tss",
	"segment not present",
	"stack-segment fault",
	"general protection",
	"page fault",
	"reservado",
	"x87 fp exception",
	"alignment check",
	"machine check",
	"simd fp exception",
	"virtualization exception",
	"control protection exception",
	"reservado", "reservado", "reservado", "reservado",
	"reservado", "reservado", "reservado", "reservado",
	"reservado", "reservado",
};

/*
 * #ts/#np/#ss/#gp compartilham o mesmo formato de error code: um
 * indice de selector (bits 15:3), a tabela que ele referencia (bit1
 * = idt, senao bit2 diz gdt/ldt) e se veio de um evento externo
 * (bit0). error code 0 quer dizer que nao foi causado por um
 * selector especifico.
 */
static void
log_selector_err(const char *fac, uint32_t err)
{
	const char *table;

	if (err == 0) {
		klog(fac, "error code 0 (nao aponta pra um selector)");
		return;
	}

	if (err & 0x02)
		table = "idt";
	else
		table = (err & 0x04) ? "ldt" : "gdt";

	klog(fac, "selector invalido: tabela=%s indice=%u%s",
	    table, err >> 3, (err & 0x01) ? " (evento externo)" : "");
}

/*
 * error code do #pf: bit0 presente/protecao, bit1 leitura/escrita,
 * bit2 kernel/usuario, bit4 busca de instrucao. cr2 tem o endereco
 * linear que faltou.
 */
static void
log_pagefault(uint32_t err)
{
	klog("pf", "endereco=0x%x %s, %s, modo %s%s",
	    rcr2(),
	    (err & 0x01) ? "protecao violada" : "pagina ausente",
	    (err & 0x02) ? "escrita" : "leitura",
	    (err & 0x04) ? "usuario" : "kernel",
	    (err & 0x10) ? ", busca de instrucao" : "");
}

/*
 * mostra os bytes em eip: sem paginacao ligada ainda, linear ==
 * fisico, entao da pra ler direto.
 */
static void
log_invalid_opcode(uint32_t eip)
{
	const unsigned char *p = (const unsigned char *)eip;

	klog("ud", "bytes em eip: %02x %02x %02x %02x %02x %02x",
	    p[0], p[1], p[2], p[3], p[4], p[5]);
}

static void
log_doublefault(void)
{
	/*
	 * sem task gate dedicado (precisa de uma tss propria com
	 * stack conhecida boa - isso e trabalho futuro), o #df cai
	 * no mesmo isr_common_stub e na mesma stack de sempre. se a
	 * causa foi estouro de stack, isso ainda pode dar ruim; e
	 * best-effort ate ter essa tss.
	 */
	klog("df", "estado da cpu pode estar corrompido (sem tss dedicada ainda)");
}

void
trap_handler(struct trapframe *tf)
{
	const char *name = (tf->vector < NEXC) ?
	    trap_name[tf->vector] : "excecao desconhecida";

	switch (tf->vector) {
	case T_DE:
		break;			/* nada alem do generico pra decodificar */
	case T_UD:
		log_invalid_opcode(tf->eip);
		break;
	case T_DF:
		log_doublefault();
		break;
	case T_TS:
	case T_NP:
	case T_SS:
	case T_GP:
		log_selector_err(name, tf->err);
		break;
	case T_PF:
		log_pagefault(tf->err);
		break;
	default:
		break;
	}

	panic("vetor %u (%s), err=0x%x eip=0x%x cs=0x%x eflags=0x%x",
	    tf->vector, name, tf->err, tf->eip, tf->cs, tf->eflags);
}

/*
 * isr_dispatch() e quem o stub de asm chama de verdade (idt_stubs.S):
 * vetor abaixo de NEXC e excecao da cpu, dali pra cima e irq de
 * hardware (pic remapeado, ver pic.h).
 */
void
isr_dispatch(struct trapframe *tf)
{
	if (tf->vector < NEXC)
		trap_handler(tf);
	else
		irq_dispatch(tf);
}
