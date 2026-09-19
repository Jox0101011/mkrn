/*
 * clock.h - relogio de boot
 *
 * md_uptime() e machine-dependent (cada arch calibra do seu jeito),
 * mas a assinatura e a mesma em todo lugar.
 */

#ifndef _SYS_CLOCK_H_
#define _SYS_CLOCK_H_

void md_uptime(unsigned long *sec, unsigned long *usec);

#endif /* !_SYS_CLOCK_H_ */
