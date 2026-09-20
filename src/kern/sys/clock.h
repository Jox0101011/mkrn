/*
 * clock.h - relogio de boot e timer periodico
 *
 * md_uptime() e machine-dependent (cada arch calibra do seu jeito),
 * mas a assinatura e a mesma em todo lugar. "ticks" e incrementado
 * por hardclock() (amd64/pit.c) a cada interrupcao do timer - base
 * pra sleep/timeout/scheduler mais pra frente.
 */

#ifndef _SYS_CLOCK_H_
#define _SYS_CLOCK_H_

#define HZ		100	/* frequencia do timer periodico (hz) */

extern volatile unsigned long ticks;

void md_uptime(unsigned long *sec, unsigned long *usec);

#endif /* !_SYS_CLOCK_H_ */
