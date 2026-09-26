/*
 * gdt.c - monta a gdt e a carrega
 *
 * descritores flat (base 0, limite = 4g inteiro): a protecao de
 * memoria de verdade fica por conta da paginacao, nao da segmentacao,
 * entao code/data de kernel e user so precisam existir pra permitir
 * trocar de ring.
 *
 * o descritor 5 (tss) e diferente dos outros: nao e code/data, e um
 * descritor de sistema apontando pra uma struct tss de verdade (ver
 * machine/segments.h). nao usamos troca de task por hardware (um
 * `ljmp`/`call` pra um descritor de tss, com a propria cpu salvando
 * registradores - lento e por fora do nosso controle); a tss serve
 * so pra uma coisa: quando uma interrupcao pega a cpu rodando em
 * ring3, ela troca de stack sozinha pra ss0:esp0 antes de empilhar
 * o trapframe - sem uma tss carregada (ltr), isso da #gp. esp0
 * precisa apontar pro topo da stack de kernel da thread CORRENTE, e
 * por isso tss_set_kstack() e chamado a cada troca de thread
 * (subr_thread.c) - o valor so importa de verdade quando tiver
 * codigo de ring3 chamando alguma interrupcao (ainda nao tem).
 */


#include "../sys/types.h"
#include "../sys/libkern.h"
#include "include/machine/cpufunc.h"
#include "include/machine/segments.h"

static struct gdt_entry gdt[NGDT];
static struct gdt_ptr gdtr;
static struct tss tss;

void gdt_flush(uint32_t gdtr_addr);	/* amd64/gdt_flush.S */

static void
gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
	gdt[i].limit_lo = limit & 0xffff;
	gdt[i].base_lo = base & 0xffff;
	gdt[i].base_mid = (base >> 16) & 0xff;
	gdt[i].access = access;
	gdt[i].limit_hi_flags = ((limit >> 16) & 0x0f) | (flags & 0xf0);
	gdt[i].base_hi = (base >> 24) & 0xff;
}

void
gdt_init(void)
{
	gdt_set(0, 0, 0, 0, 0);				/* null */

	gdt_set(1, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(0) |
	    GDT_A_SEGMENT | GDT_A_EXEC | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);			/* kernel code */

	gdt_set(2, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(0) |
	    GDT_A_SEGMENT | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);			/* kernel data */

	gdt_set(3, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(3) |
	    GDT_A_SEGMENT | GDT_A_EXEC | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);			/* user code */

	gdt_set(4, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(3) |
	    GDT_A_SEGMENT | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);			/* user data */

	memset(&tss, 0, sizeof(tss));
	tss.ss0 = GSEL_KDATA;
	tss.iomap_base = sizeof(tss);	/* >= limite: sem bitmap de io, ring3 nunca acessa porta direto */
	gdt_set(5, (uint32_t)&tss, sizeof(tss) - 1,
	    GDT_A_PRESENT | GDT_A_RING(0) | GDT_A_TSS32, 0);	/* byte granularidade, nao 4k */

	gdtr.limit = sizeof(gdt) - 1;
	gdtr.base = (uint32_t)gdt;

	gdt_flush((uint32_t)&gdtr);

	ltr(GSEL_TSS);	/* precisa vir depois do gdt_flush(): valida contra a gdt ja carregada */
}

void
tss_set_kstack(uint32_t esp0)
{
	tss.esp0 = esp0;
}
