/*
 * pmap.h - paginacao classica de 32 bits (sem pae)
 *
 *   cr3 -> page directory (1024 pde, 4 bytes cada)
 *            |
 *            +-> page table (1024 pte, 4 bytes cada)
 *                  |
 *                  +-> pagina fisica de 4096 bytes
 *
 * cada pde cobre 4mb (1024 paginas); o endereco virtual se quebra em
 * tres pedacos: bits 31-22 indexam o pde, bits 21-12 indexam o pte
 * dentro da tabela que o pde aponta, bits 11-0 sao o deslocamento
 * dentro da pagina.
 */

#ifndef _MACHINE_PMAP_H_
#define _MACHINE_PMAP_H_

#include <stdint.h>

/* bits comuns a pde e pte */
#define PTE_PRESENT	0x001
#define PTE_RW		0x002	/* 0 = somente leitura, 1 = escrita liberada */
#define PTE_USER	0x004	/* 0 = so ring0, 1 = acessivel do ring3 */
#define PTE_PWT		0x008
#define PTE_PCD		0x010
#define PTE_ACCESSED	0x020
#define PTE_DIRTY	0x040	/* so faz sentido em pte */
#define PTE_PS		0x080	/* pde: pagina de 4m - nao usamos */

#define PDE_INDEX(va)	(((uint32_t)(va) >> 22) & 0x3ff)
#define PTE_INDEX(va)	(((uint32_t)(va) >> 12) & 0x3ff)
#define PAGE_OFFSET(va)	((uint32_t)(va) & 0xfff)

#define PTE_ADDR(pte)	((uint32_t)(pte) & ~0xfff)

void pmap_init(void);

int pmap_map(uint32_t va, uint32_t pa, uint32_t flags);
void pmap_unmap(uint32_t va);
uint32_t pmap_extract(uint32_t va);	/* va -> pa, ou 0 se nao mapeado */

#endif /* !_MACHINE_PMAP_H_ */
