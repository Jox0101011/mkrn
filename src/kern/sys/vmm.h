/*
 * vmm.h - api de memoria virtual (maquina-independente)
 *
 * flags portateis pra mapear pagina. quem traduz isso pro formato
 * de pte que a arquitetura usa de verdade e a camada de baixo
 * (amd64/pmap.c) - nunca o chamador. fora do pmap, nada no kernel
 * deveria incluir machine/pmap.h nem falar em pte; e vmm_map()/
 * vmm_unmap()/vmm_extract() com essas flags aqui.
 */

#ifndef _SYS_VMM_H_
#define _SYS_VMM_H_

#include <stdint.h>

#define PAGE_PRESENT	0x01	/* pagina valida (mapear ja implica isso) */
#define PAGE_WRITE	0x02	/* 0 = somente leitura, 1 = escrita liberada */
#define PAGE_USER	0x04	/* 0 = so kernel (ring0), 1 = acessivel do ring3 */

/*
 * exemplos de combinacao:
 *   pagina de kernel:       PAGE_PRESENT | PAGE_WRITE
 *   codigo de usuario:      PAGE_PRESENT | PAGE_USER
 *   dado de usuario:        PAGE_PRESENT | PAGE_WRITE | PAGE_USER
 */

int vmm_map(uint32_t va, uint32_t pa, unsigned flags);
void vmm_unmap(uint32_t va);
uint32_t vmm_extract(uint32_t va);	/* va -> pa, ou 0 se nao mapeado */

#endif /* !_SYS_VMM_H_ */
