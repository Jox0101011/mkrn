/*
 * subr_pmm.c - gerenciador de memoria fisica (bitmap de paginas)
 *
 * o bitmap nao e mais estatico - ele mora onde pmm_init() mandar (o
 * chamador escolhe o endereco fisico e reserva esse espaco, ver
 * amd64/pmm_boot.c), do tamanho certo pra ram que a maquina tem de
 * verdade: pra 128mib (32768 paginas) isso da 4096 bytes de bitmap.
 *
 * pmm_reserve() arredonda a faixa pra fora (marca reservado de mais
 * em vez de menos, se a faixa nao cair certinho numa borda de
 * pagina); pmm_free_region() arredonda pra dentro (so libera pagina
 * inteira, nunca um pedaco que pode invadir uma regiao vizinha nao
 * declarada disponivel). assimetria proposital: errar reservando de
 * mais e seguro, errar liberando de mais nao e.
 */


#include "sys/types.h"
#include "sys/libkern.h"
#include "sys/pmm.h"

static uint8_t *bitmap;
static unsigned long npages_total;
static unsigned long npages_free;
static unsigned long alloc_cursor;

static __inline void
bit_set(unsigned long page)
{
	bitmap[page >> 3] |= (uint8_t)(1 << (page & 7));
}

static __inline void
bit_clear(unsigned long page)
{
	bitmap[page >> 3] &= (uint8_t)~(1 << (page & 7));
}

static __inline int
bit_test(unsigned long page)
{
	return (bitmap[page >> 3] >> (page & 7)) & 1;
}

void
pmm_init(void *bitmap_addr, unsigned long npages)
{
	unsigned long bytes = (npages + 7) / 8;

	bitmap = (uint8_t *)bitmap_addr;
	npages_total = npages;
	npages_free = 0;
	alloc_cursor = 0;

	/* tudo comeca ocupado */
	memset(bitmap, 0xff, bytes);
}

void
pmm_reserve(uint64_t base, uint64_t len)
{
	uint64_t start, end;
	unsigned long p, plast;

	if (len == 0)
		return;

	start = base & ~((uint64_t)PAGE_SIZE - 1);
	end = (base + len + PAGE_SIZE - 1) & ~((uint64_t)PAGE_SIZE - 1);

	plast = (unsigned long)(end / PAGE_SIZE);
	if (plast > npages_total)
		plast = npages_total;

	for (p = (unsigned long)(start / PAGE_SIZE); p < plast; p++) {
		if (!bit_test(p)) {
			bit_set(p);
			npages_free--;
		}
	}
}

void
pmm_free_region(uint64_t base, uint64_t len)
{
	uint64_t start, end;
	unsigned long p, plast;

	if (len == 0)
		return;

	start = (base + PAGE_SIZE - 1) & ~((uint64_t)PAGE_SIZE - 1);
	if (base + len < start)
		return;		/* menor que uma pagina inteira: nada a liberar */
	end = (base + len) & ~((uint64_t)PAGE_SIZE - 1);

	plast = (unsigned long)(end / PAGE_SIZE);
	if (plast > npages_total)
		plast = npages_total;

	for (p = (unsigned long)(start / PAGE_SIZE); p < plast; p++) {
		if (bit_test(p)) {
			bit_clear(p);
			npages_free++;
		}
	}
}

uint32_t
pmm_alloc(void)
{
	unsigned long p, i;

	for (i = 0; i < npages_total; i++) {
		p = alloc_cursor;
		alloc_cursor = (alloc_cursor + 1 < npages_total) ? alloc_cursor + 1 : 0;

		if (!bit_test(p)) {
			bit_set(p);
			npages_free--;
			return (uint32_t)(p * PAGE_SIZE);
		}
	}

	return PMM_ENOMEM;
}

void
pmm_free(uint32_t paddr)
{
	unsigned long p = paddr / PAGE_SIZE;

	if (p >= npages_total)
		return;

	if (bit_test(p)) {
		bit_clear(p);
		npages_free++;
	}
}

unsigned long
pmm_npages(void)
{
	return npages_total;
}

unsigned long
pmm_nfree(void)
{
	return npages_free;
}
