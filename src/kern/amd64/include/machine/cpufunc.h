/*
 * cpufunc.h - primitivas de baixo nivel do amd64
 */

#ifndef _MACHINE_CPUFUNC_H_
#define _MACHINE_CPUFUNC_H_

#include "../../../sys/types.h"


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

/* controles gerais (cr0.pg liga paginacao) e o endereco do page directory */
static __inline uint32_t
rcr0(void)
{
	uint32_t val;

	__asm__ volatile("mov %%cr0, %0" : "=r"(val));
	return val;
}

static __inline void
lcr0(uint32_t val)
{
	__asm__ volatile("mov %0, %%cr0" : : "r"(val) : "memory");
}

static __inline uint32_t
rcr3(void)
{
	uint32_t val;

	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val;
}

static __inline void
lcr3(uint32_t val)
{
	__asm__ volatile("mov %0, %%cr3" : : "r"(val) : "memory");
}

/* derruba uma entrada da tlb depois de mudar o mapeamento dela */
static __inline void
invlpg(uint32_t va)
{
	__asm__ volatile("invlpg (%0)" : : "r"(va) : "memory");
}

static __inline void
sti(void)
{
	__asm__ volatile("sti");
}

static __inline void
cli(void)
{
	__asm__ volatile("cli");
}

/* carrega o task register - so isso, o descritor de tss ja precisa
   estar na gdt carregada (ver amd64/gdt.c) */
static __inline void
ltr(uint16_t sel)
{
	__asm__ volatile("ltr %0" : : "r"(sel));
}

/*
 * desliga interrupcoes e devolve o eflags de antes, pra restaurar
 * depois com sti_restore() - diferente de um cli()/sti() as cegas,
 * isso nao liga interrupcao que ja estava desligada (por exemplo,
 * se quem chamou ja estava dentro de um isr). usado pra proteger
 * secoes criticas curtas (heap, console) contra o timer preemptando
 * no meio (subr_thread.c: scheduler_tick()) e trocando pra outra
 * thread que mexe na mesma coisa - sem isso duas threads podem
 * cair dentro do kmalloc()/klog() "ao mesmo tempo".
 */
static __inline uint32_t
cli_save(void)
{
	uint32_t eflags;

	__asm__ volatile("pushfl; popl %0; cli" : "=r"(eflags) :: "memory");
	return eflags;
}

static __inline void
sti_restore(uint32_t eflags)
{
	__asm__ volatile("pushl %0; popfl" : : "r"(eflags) : "memory", "cc");
}

#endif /* !_MACHINE_CPUFUNC_H_ */
