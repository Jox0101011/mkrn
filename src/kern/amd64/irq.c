/*
 * irq.c - despacho de interrupcoes de hardware (vetores 32-47)
 *
 * irq_install() so registra - nao desmascara a irq no pic sozinho,
 * isso fica explicito por conta de quem chama (pic_unmask()).
 */


#include "../sys/types.h"
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

	/* checa espuria ANTES de tratar - nao faz sentido chamar o
	   handler registrado pra um evento que na pratica nao aconteceu */
	if (pic_is_spurious(irq)) {
		klog("irq", "irq %u espuria, ignorada", irq);
		return;
	}

	/*
	 * eoi ANTES do handler, nao depois: o handler do timer chama
	 * scheduler_tick(), que pode trocar de thread ali dentro - se
	 * isso acontecer, essa chamada so "volta" (e chega no eoi)
	 * quando essa mesma thread for escalonada de novo, que pode
	 * ser bem mais tarde. o pic ficaria com essa irq marcada como
	 * "em atendimento" o tempo todo, travando novos ticks - o
	 * timer nunca mais preemptaria ninguem.
	 */
	pic_eoi(irq);

	if (irq < NIRQ && irq_table[irq] != NULL)
		irq_table[irq](tf);
	else
		klog("irq", "irq %u sem handler instalado", irq);
}
