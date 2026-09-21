/*
 * idt.c - monta a idt
 *
 * vetores 0-31 (excecoes) e 32-47 (irqs de hardware, pic remapeado)
 * apontam pros stubs de amd64/idt_stubs.S. 48-255 ficam sem handler
 * (present=0) por enquanto - isso e coisa de apic/syscall. o que
 * fazer com cada vetor e decidido em trap.c/irq.c, esse arquivo so
 * monta a tabela.
 */


#include "../sys/types.h"
#include "include/machine/idt.h"
#include "include/machine/pic.h"
#include "include/machine/segments.h"

extern uint32_t isr_stub_table[NEXC + NIRQ];	/* amd64/idt_stubs.S */

static struct idt_entry idt[NIDT];
static struct idt_ptr idtr;

static void
idt_set(int vector, uint32_t base, uint16_t selector, uint8_t type_attr)
{
	idt[vector].base_lo = base & 0xffff;
	idt[vector].selector = selector;
	idt[vector].zero = 0;
	idt[vector].type_attr = type_attr;
	idt[vector].base_hi = (base >> 16) & 0xffff;
}

void
idt_init(void)
{
	int i;

	for (i = 0; i < NEXC + NIRQ; i++)
		idt_set(i, isr_stub_table[i], GSEL_KCODE,
		    IDT_A_PRESENT | IDT_A_RING(0) | IDT_T_INT32);

	for (; i < NIDT; i++)
		idt_set(i, 0, 0, 0);

	idtr.limit = sizeof(idt) - 1;
	idtr.base = (uint32_t)idt;

	__asm__ volatile("lidt %0" : : "m"(idtr));
}
