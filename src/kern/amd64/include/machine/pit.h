/*
 * pit.h - 8253/8254 (pit), canal 0 como relogio periodico
 *
 * PIT_HZ e a frequencia do cristal do chip - vale tanto pro canal 0
 * (timer periodico daqui) quanto pro canal 2 (calibracao do tsc,
 * amd64/tsc.c).
 */

#ifndef _MACHINE_PIT_H_
#define _MACHINE_PIT_H_

#define PIT_HZ		1193182

void pit_init(unsigned hz);

#endif /* !_MACHINE_PIT_H_ */
