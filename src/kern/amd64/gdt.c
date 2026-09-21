/*
 * gdt.c - monta a gdt e a carrega
 *
 * descritores flat (base 0, limite = 4g inteiro): a protecao de
 * memoria de verdade fica por conta da paginacao, nao da segmentacao,
 * entao code/data de kernel e user so precisam existir pra permitir
 * trocar de ring.
 */


#include "../sys/types.h"
#include "include/machine/segments.h"

static struct gdt_entry gdt[NGDT];
static struct gdt_ptr gdtr;

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
	gdt_set(0, 0, 0, 0, 0);					/* null */

	gdt_set(1, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(0) |
	    GDT_A_SEGMENT | GDT_A_EXEC | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);				/* kernel code */

	gdt_set(2, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(0) |
	    GDT_A_SEGMENT | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);				/* kernel data */

	gdt_set(3, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(3) |
	    GDT_A_SEGMENT | GDT_A_EXEC | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);				/* user code */

	gdt_set(4, 0, 0xfffff, GDT_A_PRESENT | GDT_A_RING(3) |
	    GDT_A_SEGMENT | GDT_A_RW,
	    GDT_F_GRAN_4K | GDT_F_SZ_32);				/* user data */

	gdt_set(5, 0, 0, 0, 0);		/* tss: preenchida numa proxima etapa */

	gdtr.limit = sizeof(gdt) - 1;
	gdtr.base = (uint32_t)gdt;

	gdt_flush((uint32_t)&gdtr);
}
