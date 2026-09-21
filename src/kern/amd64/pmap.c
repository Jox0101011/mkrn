/*
 * pmap.c - monta o page directory definitivo do kernel
 *
 * a paginacao ja foi ligada em amd64/boot.S, com uma tabela
 * temporaria que so cobre os primeiros 4m (identity + higher half).
 * aqui, com o pmm de pe, montamos a tabela de verdade e trocamos o
 * cr3 - a troca e segura porque essa tabela nova tambem mapeia o
 * proprio kernel no higher half, entao o codigo que esta executando
 * agora (isso aqui) continua acessivel depois do lcr3().
 *
 * NAO mapeamos identidade de toda a ram (uma versao anterior fazia
 * isso e tinha dois bugs de verdade por causa disso):
 *
 *   1. construir esse mapa gigante pede pagina de tabela nova via
 *      pmm_alloc() centenas de vezes: cada pagina fisica devolvida e
 *      escrita diretamente pelo proprio endereco (memset+pde/pte),
 *      mas enquanto isso o cr3 AINDA e o temporario do boot.S, que
 *      so cobre 4m! assim que o pmm_alloc() comeca a devolver pagina
 *      alem de 4m (inevitavel com ram grande o suficiente), essa
 *      escrita da #pf na hora.
 *
 *   2. identity map de toda a ram colide com o proprio higher half:
 *      kernbase e 0xc0000000 (3g) - com mais de ~3g de ram, o
 *      endereco fisico (por exemplo) 3.2g mapeado por identidade cai
 *      EM CIMA do range de enderecos virtuais que o kernel/heap/etc
 *      ja estao usando.
 *
 * a solucao: so mapeamos uma faixa baixa fixa (pequena, cabe o
 * kernel + bitmap do pmm + vga em qualquer tamanho de ram) e o alias
 * do kernel no higher half. quem precisar tocar uma pagina fisica
 * especifica depois disso (mmio, por exemplo) usa vmm_map() na hora
 * - a essa altura pmm/vmm ja estao de pe e cr3 ja e o definitivo,
 * entao nao tem mais o problema do item 1.
 */

#include "../sys/types.h"
#include "include/machine/cpufunc.h"
#include "include/machine/pmap.h"
#include "../sys/libkern.h"
#include "../sys/log.h"
#include "../sys/panic.h"
#include "../sys/pmm.h"

extern char kernel_start[];	/* amd64/kern.lds, fisico */
extern char kernel_end[];

/* 8m: kernel + bitmap do pmm (no maximo 128k, ram sempre cabe em 4g
   sem pae) + vga (0xb8000) cabem folgado aqui, qualquer que seja o
   tamanho de ram da maquina - ver o comentario grande acima */
#define PMAP_LOW_END	0x800000u

/* page directory do kernel - a faixa baixa fica identity-mapeada,
   entao o proprio endereco fisico serve de ponteiro pra ele */
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

	/*
	 * a permissao efetiva de um acesso e a INTERSECAO do pde com o
	 * pte - se o pde nao tiver PTE_USER, uma pte com PAGE_USER
	 * simplesmente nao funciona, mesmo com o bit dela certo. entao
	 * o pde sempre libera geral (present+rw+user) e quem restringe
	 * de verdade e SEMPRE a pte individual (pmap_map() abaixo),
	 * igual todo kernel de verdade faz.
	 */
	pgdir[pde_index] = pt_phys | PTE_PRESENT | PTE_RW | PTE_USER;

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
	uint32_t pd_phys, addr, low_end;
	uint64_t ram_bytes;

	pd_phys = pmm_alloc();
	if (pd_phys == PMM_ENOMEM)
		panic("pmap: sem pagina livre pro page directory");

	pgdir = (uint32_t *)pd_phys;
	memset(pgdir, 0, PAGE_SIZE);

	/* nao passa do que a maquina realmente tem, pra maquinas com
	   bem menos que 8m de ram */
	ram_bytes = (uint64_t)pmm_npages() * PAGE_SIZE;
	low_end = (ram_bytes < PMAP_LOW_END) ? (uint32_t)ram_bytes : PMAP_LOW_END;

	/* identity map de uma faixa baixa fixa, comecando em PAGE_SIZE
	   (nunca 0) - um desvio de ponteiro nulo continua dando #pf de
	   verdade em vez de ler/escrever na pagina fisica 0 */
	for (addr = PAGE_SIZE; addr < low_end; addr += PAGE_SIZE)
		pmap_map(addr, addr, PTE_RW);

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

	klog("pmap", "baixa 0x%x-0x%x mapeada 1:1 (sem a pagina 0), kernel tambem em 0x%x-0x%x, pd em 0x%x",
	    (uint32_t)PAGE_SIZE, low_end,
	    KERNBASE + (uint32_t)kernel_start, KERNBASE + (uint32_t)kernel_end, pd_phys);
}
