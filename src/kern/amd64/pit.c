/*
 * pit.c - canal 0 do pit como relogio periodico (irq0)
 *
 *   irq0 -> hardclock() -> ticks++ -> scheduler_tick() (sys/thread.h)
 *
 * scheduler_tick() e o que torna o escalonador preemptivo: cada
 * disparo do pit e uma fatia de tempo, a thread rodando nao escolhe
 * quando solta a cpu (diferente de um yield() cooperativo).
 *
 * modo 3 (square wave generator): o canal conta o divisor pra baixo
 * a partir do cristal (PIT_HZ) e dispara a irq0 de novo toda vez que
 * chega em zero - da um pulso periodico sem precisar reprogramar
 * nada depois do init.
 */


#include "../sys/types.h"
#include "include/machine/cpufunc.h"
#include "include/machine/idt.h"
#include "include/machine/pic.h"
#include "include/machine/pit.h"
#include "../sys/clock.h"
#include "../sys/thread.h"

#define PIT_CH0		0x40
#define PIT_CMD		0x43

#define PIT_CMD_CH0	0x00	/* seleciona o canal 0 */
#define PIT_CMD_LOHI	0x30	/* le/escreve o divisor em dois bytes: lo, hi */
#define PIT_CMD_MODE3	0x06	/* modo 3: square wave generator */

volatile unsigned long ticks;

static void
hardclock(struct trapframe *tf)
{
	(void)tf;
	ticks++;
	scheduler_tick();	/* uma fatia de tempo por irq0 - ver subr_thread.c */
}

void
pit_init(unsigned hz)
{
	uint16_t divisor = PIT_HZ / hz;

	outb(PIT_CMD, PIT_CMD_CH0 | PIT_CMD_LOHI | PIT_CMD_MODE3);
	outb(PIT_CH0, divisor & 0xff);
	outb(PIT_CH0, (divisor >> 8) & 0xff);

	irq_install(0, hardclock);
	pic_unmask(0);
}
