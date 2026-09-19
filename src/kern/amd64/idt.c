/*
 * idt.c - monta a idt e trata as excecoes da cpu
 *
 * vetores 0-31 apontam pros stubs de amd64/idt.S. 32-255 ficam sem
 * handler (present=0) por enquanto - isso e coisa de pic/apic e
 * syscall, que ainda nao existem.
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/idt.h"
#include "include/machine/segments.h"
#include "../sys/panic.h"

extern uint32_t isr_stub_table[NEXC];	/* amd64/idt_stubs.S */

static struct idt_entry idt[NIDT];
static struct idt_ptr idtr;

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

	for (i = 0; i < NEXC; i++)
		idt_set(i, isr_stub_table[i], GSEL_KCODE,
		    IDT_A_PRESENT | IDT_A_RING(0) | IDT_T_INT32);

	for (; i < NIDT; i++)
		idt_set(i, 0, 0, 0);

	idtr.limit = sizeof(idt) - 1;
	idtr.base = (uint32_t)idt;

	__asm__ volatile("lidt %0" : : "m"(idtr));
}

void
trap_handler(struct trapframe *tf)
{
	const char *name = "excecao desconhecida";

	if (tf->vector < NEXC)
		name = trap_name[tf->vector];

	panic("vetor %u (%s), err=0x%x eip=0x%x cs=0x%x eflags=0x%x",
	    tf->vector, name, tf->err, tf->eip, tf->cs, tf->eflags);
}
