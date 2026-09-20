/*
 * subr_vmm.c - traduz as flags portateis de sys/vmm.h pro formato de
 * pte que a arquitetura usa (amd64/pmap.c)
 *
 * este e o unico arquivo fora do proprio pmap que inclui
 * machine/pmap.h - o resto do kernel so deveria falar a lingua
 * generica (PAGE_PRESENT/PAGE_WRITE/PAGE_USER), nunca PTE_*.
 */

#include "amd64/include/machine/pmap.h"
#include "sys/vmm.h"

static uint32_t
pte_flags(unsigned flags)
{
	uint32_t f = 0;

	if (flags & PAGE_WRITE)
		f |= PTE_RW;
	if (flags & PAGE_USER)
		f |= PTE_USER;

	return f;
}

int
vmm_map(uint32_t va, uint32_t pa, unsigned flags)
{
	return pmap_map(va, pa, pte_flags(flags));
}

void
vmm_unmap(uint32_t va)
{
	pmap_unmap(va);
}

uint32_t
vmm_extract(uint32_t va)
{
	return pmap_extract(va);
}
