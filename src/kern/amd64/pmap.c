/*
 * pmap.c - monta o page directory definitivo do kernel
 *
 * a paginacao ja foi ligada em amd64/boot.S, com uma tabela
 * temporaria que so cobre os primeiros 4m (identity + higher half).
 * aqui, com o pmm de pe, montamos a tabela de verdade cobrindo toda
 * a ram conhecida e trocamos o cr3 - a troca e segura porque essa
 * tabela nova tambem mapeia o proprio kernel no higher half, entao
 * o codigo que esta executando agora (isso aqui) continua acessivel
 * depois do lcr3().
 *
 * por enquanto so existe um espaco de enderecamento (nao tem
 * processos ainda), entao um unico page directory estatico serve.
 * mantemos o mapa de identidade (va == pa) de tudo que o pmm
 * conhece, alem do alias do kernel em kernbase+ - o identity map
 * continua util pro kernel acessar qualquer pagina fisica pelo
 * proprio endereco (mmio, bitmap do pmm, etc).
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/cpufunc.h"
#include "include/machine/pmap.h"
#include "../sys/libkern.h"
#include "../sys/log.h"
#include "../sys/panic.h"
#include "../sys/pmm.h"

extern char kernel_start[];	/* amd64/kern.ld, fisico */
extern char kernel_end[];

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

	/* identity map: da pro kernel acessar qualquer pagina fisica
	   que o pmm conhece pelo proprio endereco (vga, mmio, etc) */
	npages = pmm_npages();
	for (i = 0; i < npages; i++) {
		addr = (uint32_t)(i * PAGE_SIZE);
		pmap_map(addr, addr, PTE_RW);
	}

	/* alias do kernel no higher half - e daqui que o codigo que
	   esta rodando agora (isso aqui, kmain, etc) continua sendo
	   buscado depois do lcr3() abaixo */
	for (addr = (uint32_t)kernel_start; addr < (uint32_t)kernel_end;
	    addr += PAGE_SIZE)
		pmap_map(KERNBASE + addr, addr, PTE_RW);

	/* troca a tabela temporaria (amd64/boot.S, so cobria 4m) pela
	   definitiva - a paginacao ja estava ligada desde o boot, isso
	   so troca qual tabela o cr3 aponta */
	lcr3(pd_phys);

	klog("pmap", "%lu paginas mapeadas 1:1, kernel tambem em 0x%x-0x%x, pd em 0x%x",
	    npages, KERNBASE + (uint32_t)kernel_start,
	    KERNBASE + (uint32_t)kernel_end, pd_phys);
}
