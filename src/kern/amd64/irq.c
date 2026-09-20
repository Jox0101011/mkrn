/*
 * irq.c - despacho de interrupcoes de hardware (vetores 32-47)
 *
 * irq_install() so registra - nao desmascara a irq no pic sozinho,
 * isso fica explicito por conta de quem chama (pic_unmask()).
 */

#include <stddef.h>

#include "include/machine/idt.h"
#include "include/machine/pic.h"
#include "../sys/log.h"

static irq_handler_t irq_table[NIRQ];

void
irq_install(unsigned irq, irq_handler_t handler)
{
	if (irq < NIRQ)
		irq_table[irq] = handler;
}

void
irq_dispatch(struct trapframe *tf)
{
	unsigned irq = tf->vector - IRQ_BASE;

	if (irq < NIRQ && irq_table[irq] != NULL)
		irq_table[irq](tf);
	else
		klog("irq", "irq %u sem handler instalado", irq);

	pic_eoi(irq);
}
