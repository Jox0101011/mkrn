/*
 * cpufunc.h - primitivas de baixo nivel do amd64
 */

#ifndef _MACHINE_CPUFUNC_H_
#define _MACHINE_CPUFUNC_H_

#include <stdint.h>

static __inline void
outb(uint16_t port, uint8_t val)
{
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static __inline uint8_t
inb(uint16_t port)
{
	uint8_t val;

	__asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
	return val;
}

static __inline void
outw(uint16_t port, uint16_t val)
{
	__asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static __inline uint16_t
inw(uint16_t port)
{
	uint16_t val;

	__asm__ volatile("inw %1, %0" : "=a"(val) : "Nd"(port));
	return val;
}

static __inline uint64_t
rdtsc(void)
{
	uint32_t lo, hi;

	__asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

/* endereco linear que causou o ultimo #pf */
static __inline uint32_t
rcr2(void)
{
	uint32_t val;

	__asm__ volatile("mov %%cr2, %0" : "=r"(val));
	return val;
}

#endif /* !_MACHINE_CPUFUNC_H_ */
