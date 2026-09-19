/*
 * idt.h - idt: gates de interrupcao/excecao
 */

#ifndef _MACHINE_IDT_H_
#define _MACHINE_IDT_H_

#include <stdint.h>

#define NIDT		256	/* a idt tem ate 256 vetores */
#define NEXC		32	/* vetores 0-31 sao excecoes da cpu */

/* gate de interrupcao (8 bytes) */
struct idt_entry {
	uint16_t	base_lo;
	uint16_t	selector;
	uint8_t		zero;
	uint8_t		type_attr;
	uint16_t	base_hi;
} __attribute__((packed));

/* operando do lidt */
struct idt_ptr {
	uint16_t	limit;
	uint32_t	base;
} __attribute__((packed));

/* type_attr: bit7 p, bits6-5 dpl, bit4 sempre 0 nos gates,
   bits3-0 tipo (0xe = interrupt gate de 32 bits) */
#define IDT_A_PRESENT	0x80
#define IDT_A_RING(x)	(((x) & 3) << 5)
#define IDT_T_INT32	0x0e

/*
 * estado empilhado por isr_common_stub (amd64/idt.S) antes de chamar
 * trap_handler(): pusha (sem os segmentos, o kernel so roda em ring0
 * por enquanto e ds/es/fs/gs nunca mudam) + vector/err que o stub da
 * excecao empilhou + o que a propria cpu empilha na excecao.
 */
struct trapframe {
	uint32_t	edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
	uint32_t	vector;
	uint32_t	err;
	uint32_t	eip, cs, eflags;
};

void idt_init(void);
void trap_handler(struct trapframe *tf);

#endif /* !_MACHINE_IDT_H_ */
