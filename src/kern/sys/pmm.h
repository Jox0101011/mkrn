/*
 * pmm.h - gerenciador de memoria fisica (pmm)
 *
 * bitmap de 1 bit por pagina de 4096 bytes: 1 = ocupada, 0 = livre.
 * pmm_init() recebe onde o bitmap deve morar fisicamente (quem
 * escolhe o endereco e reserva esse espaco e o chamador, ver
 * amd64/pmm_boot.c) e quantas paginas ele cobre; comeca tudo
 * marcado como ocupado - so fica livre o que pmm_free_region()
 * liberar explicitamente. isso reflete o memory map do bootloader:
 * regiao que ninguem declarou disponivel (firmware, mmio, acpi, rom,
 * framebuffer, ou simplesmente memoria que nao existe na maquina)
 * fica reservada por padrao, nunca o contrario.
 */

#ifndef _SYS_PMM_H_
#define _SYS_PMM_H_

#include <stdint.h>

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12

#define PMM_ENOMEM	((uint32_t)-1)	/* retorno de pmm_alloc() sem pagina livre */

void pmm_init(void *bitmap_addr, unsigned long npages);

void pmm_reserve(uint64_t base, uint64_t len);
void pmm_free_region(uint64_t base, uint64_t len);

uint32_t pmm_alloc(void);
void pmm_free(uint32_t paddr);

unsigned long pmm_npages(void);
unsigned long pmm_nfree(void);

#endif /* !_SYS_PMM_H_ */
