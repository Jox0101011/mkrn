/*
 * pic.h - 8259a (pic) e despacho de irq
 *
 * irq0-15 sao remapeados pros vetores 32-47, logo depois das
 * excecoes da cpu (0-31) - dessa forma nao colidem.
 */

#ifndef _MACHINE_PIC_H_
#define _MACHINE_PIC_H_

#include "idt.h"	/* struct trapframe */

#define PIC1_CMD	0x20	/* mestre: irq0-7 */
#define PIC1_DATA	0x21
#define PIC2_CMD	0xa0	/* escrava: irq8-15 */
#define PIC2_DATA	0xa1

#define IRQ_BASE	32	/* irq0 vira o vetor 32 apos o remap */
#define NIRQ		16	/* vetores 32-47 */

typedef void (*irq_handler_t)(struct trapframe *tf);

void pic_init(void);
void pic_eoi(unsigned irq);
void pic_mask(unsigned irq);
void pic_unmask(unsigned irq);

void irq_install(unsigned irq, irq_handler_t handler);
void irq_dispatch(struct trapframe *tf);

#endif /* !_MACHINE_PIC_H_ */
