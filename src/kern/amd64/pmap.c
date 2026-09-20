/*
 * pmap.c - monta o page directory do kernel e liga a paginacao
 *
 * por enquanto so existe um espaco de enderecamento (nao tem
 * processos ainda), entao um unico page directory estatico serve.
 * pmap_init() mapeia identidade (va == pa) tudo que o pmm conhece -
 * inclui o kernel, o bitmap, a memoria de video (0xb8000) e ate as
 * paginas reservadas (nao custa nada mapear, e evita ter que tratar
 * buraco no meio do range). sem esse mapeamento, a proxima instrucao
 * depois de ligar cr0.pg da #pf na hora.
 *
 * como td o range que o pmm rastreia fica identity-mapeado, qualquer
 * pagina que pmm_alloc() devolver dali em diante (pra uma tabela
 * nova, por exemplo) ja vem acessivel no proprio endereco fisico -
 * nao tem problema de galinha e ovo.
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/cpufunc.h"
#include "include/machine/pmap.h"
#include "../sys/libkern.h"
#include "../sys/log.h"
#include "../sys/panic.h"
#include "../sys/pmm.h"

/* page directory do kernel - identity-mapeado, entao o proprio
   endereco fisico serve de ponteiro o tempo todo */
static uint32_t *pgdir;

static uint32_t *
pt_for(uint32_t pde_index, int create)
{
	uint32_t pt_phys;

	if (pgdir[pde_index] & PTE_PRESENT)
		return (uint32_t *)PTE_ADDR(pgdir[pde_index]);

	if (!create)
		return NULL;

	pt_phys = pmm_alloc();
	if (pt_phys == PMM_ENOMEM)
		panic("pmap: sem pagina livre pra tabela nova");

	memset((void *)pt_phys, 0, PAGE_SIZE);
	pgdir[pde_index] = pt_phys | PTE_PRESENT | PTE_RW;

	return (uint32_t *)pt_phys;
}

int
pmap_map(uint32_t va, uint32_t pa, uint32_t flags)
{
	uint32_t *pt = pt_for(PDE_INDEX(va), 1);

	pt[PTE_INDEX(va)] = PTE_ADDR(pa) | (flags & 0xfff) | PTE_PRESENT;
	invlpg(va);

	return 0;
}

void
pmap_unmap(uint32_t va)
{
	uint32_t *pt = pt_for(PDE_INDEX(va), 0);

	if (pt == NULL)
		return;

	pt[PTE_INDEX(va)] = 0;
	invlpg(va);
}

uint32_t
pmap_extract(uint32_t va)
{
	uint32_t *pt = pt_for(PDE_INDEX(va), 0);

	if (pt == NULL || !(pt[PTE_INDEX(va)] & PTE_PRESENT))
		return 0;

	return PTE_ADDR(pt[PTE_INDEX(va)]) | PAGE_OFFSET(va);
}

void
pmap_init(void)
{
	uint32_t pd_phys, addr;
	unsigned long npages, i;

	pd_phys = pmm_alloc();
	if (pd_phys == PMM_ENOMEM)
		panic("pmap: sem pagina livre pro page directory");

	pgdir = (uint32_t *)pd_phys;
	memset(pgdir, 0, PAGE_SIZE);

	npages = pmm_npages();
	for (i = 0; i < npages; i++) {
		addr = (uint32_t)(i * PAGE_SIZE);
		pmap_map(addr, addr, PTE_RW);
	}

	lcr3(pd_phys);
	lcr0(rcr0() | 0x80000000);	/* cr0.pg */

	klog("pmap", "paginacao ligada: %lu paginas mapeadas 1:1, pd em 0x%x",
	    npages, pd_phys);
}
