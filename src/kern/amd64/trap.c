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
 * handler de #pf (vetor 14) - as tres informacoes que interessam:
 *
 *   cr2   endereco linear que causou o fault
 *   err   motivo + tipo de acesso (bits abaixo)
 *   eip   instrucao que estava executando (aqui e eip, nao rip -
 *         o kernel ainda roda em modo protegido de 32 bits, sem
 *         paginacao de 64 bits/long mode; cr2 tambem e so os 32
 *         bits baixos por isso, nao um valor de 64 bits completo)
 *
 * bits do error code (intel sdm vol.3 4.7):
 *   bit 0 (P)     0 = pagina ausente, 1 = violacao de protecao
 *   bit 1 (W/R)   0 = leitura, 1 = escrita
 *   bit 2 (U/S)   0 = kernel, 1 = user mode
 *   bit 3 (RSVD)  1 = bit reservado setado indevidamente numa pde/pte
 *   bit 4 (I/D)   1 = fault veio de busca de instrucao
 *
 * hoje todo #pf e fatal - nao existe demand paging, stack que
 * cresce nem copy-on-write ainda, entao nao ha o que fazer alem de
 * logar direito e chamar panic(). o decode fica pronto pra quando
 * essas coisas existirem (ex.: PF_PRESENT desligado + endereco
 * dentro de uma vma valida = aloca e mapeia em vez de panicar).
 */
#define PF_PRESENT	0x01
#define PF_WRITE	0x02
#define PF_USER		0x04
#define PF_RSVD		0x08
#define PF_INSTR	0x10

static void
pagefault_handler(struct trapframe *tf)
{
	uint32_t addr = rcr2();

	klog("pf", "cr2=0x%x eip=0x%x err=0x%x", addr, tf->eip, tf->err);
	klog("pf", "%s, %s, modo %s%s%s",
	    (tf->err & PF_PRESENT) ? "protecao violada" : "pagina ausente",
	    (tf->err & PF_WRITE) ? "escrita" : "leitura",
	    (tf->err & PF_USER) ? "usuario" : "kernel",
	    (tf->err & PF_INSTR) ? ", busca de instrucao" : "",
	    (tf->err & PF_RSVD) ? ", bit reservado invalido numa pde/pte" : "");
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
		pagefault_handler(tf);
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
