/*
 * tsc.c - relogio de boot baseado no tsc
 *
 * sem timer/interrupcoes ainda, entao o tempo de log e derivado do
 * tsc: calibramos a frequencia dele contra o pit (canal 2, um pulso
 * de 50ms) e depois so comparamos com o valor lido no boot.
 *
 * isso e uma solucao provisoria; da pra trocar por um driver de
 * timer (pit/hpet/apic) mais pra frente sem mexer em quem chama
 * md_uptime().
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/cpufunc.h"
#include "include/machine/pit.h"
#include "../sys/clock.h"

#define CAL_MS		50
#define CAL_LATCH	((PIT_HZ * CAL_MS) / 1000)

static uint64_t tsc_hz;
static uint64_t tsc_boot;

/*
 * divisao inteira de 64 bits, feita na mao (deslocar e subtrair).
 * evita depender de __udivdi3/__umoddi4 do libgcc, que nao esta
 * disponivel pro alvo de 32 bits neste ambiente de build.
 */
static uint64_t
udiv64(uint64_t n, uint64_t d, uint64_t *rem)
{
	uint64_t q, r;
	int i;

	q = 0;
	r = 0;
	for (i = 63; i >= 0; i--) {
		r <<= 1;
		r |= (n >> i) & 1;
		if (r >= d) {
			r -= d;
			q |= (uint64_t)1 << i;
		}
	}

	if (rem != NULL)
		*rem = r;
	return q;
}

static uint64_t
pit_calibrate_tsc(void)
{
	uint64_t t0, t1;

	/* liga o gate do canal 2, desliga o speaker */
	outb(0x61, (inb(0x61) & 0xfc) | 0x01);

	/* canal 2, acesso lo/hi, modo 0 (one-shot) */
	outb(0x43, 0xb0);
	outb(0x42, CAL_LATCH & 0xff);
	outb(0x42, (CAL_LATCH >> 8) & 0xff);

	t0 = rdtsc();
	while ((inb(0x61) & 0x20) == 0)	/* bit 5 = out do canal 2 */
		continue;
	t1 = rdtsc();

	return (t1 - t0) * 1000 / CAL_MS;
}

void
tsc_calibrate(void)
{
	tsc_hz = pit_calibrate_tsc();
	tsc_boot = rdtsc();
}

void
md_uptime(unsigned long *sec, unsigned long *usec)
{
	uint64_t cycles, cycles_per_us, us, rem;

	cycles = rdtsc() - tsc_boot;
	cycles_per_us = udiv64(tsc_hz, 1000000, NULL);
	us = udiv64(cycles, cycles_per_us, NULL);

	*sec = (unsigned long)udiv64(us, 1000000, &rem);
	*usec = (unsigned long)rem;
}
