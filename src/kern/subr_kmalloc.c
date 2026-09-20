/*
 * subr_kmalloc.c - heap do kernel (allocator de blocos, sem sofisticacao)
 *
 * kheap_init() reserva uma faixa fixa de endereco virtual e mapeia
 * cada pagina dela na hora (pmm_alloc + vmm_map) - nao cresce
 * sozinho ainda, isso fica pra quando precisar de verdade.
 *
 * dentro dessa faixa, cada bloco (livre ou ocupado) tem um header
 * na frente dos dados, e os headers formam uma lista encadeada na
 * ordem fisica da memoria:
 *
 *   [header][data][header][data][header][data livre]
 *
 * kmalloc() e first-fit: pega o primeiro bloco livre grande o
 * suficiente e divide em dois se sobrar espaco que valha a pena.
 * kfree() marca livre e funde com o vizinho seguinte e/ou anterior
 * se tambem estiverem livres, pra nao fragmentar a toa.
 */

#include <stddef.h>
#include <stdint.h>

#include "sys/kmalloc.h"
#include "sys/log.h"
#include "sys/panic.h"
#include "sys/pmm.h"
#include "sys/vmm.h"

#define HEAP_BASE	0xd1000000u
#define HEAP_PAGES	256			/* 1mb pra comecar */
#define HEAP_SIZE	(HEAP_PAGES * PAGE_SIZE)

#define HDR_MAGIC	0x6b6d616cu		/* "kmal", detecta corrupcao/mau uso */
#define HEAP_ALIGN	16

#define ALIGN_UP(x, a)	(((x) + (a) - 1) & ~((size_t)(a) - 1))

struct kmalloc_hdr {
	uint32_t		magic;
	size_t			size;	/* tamanho da area de dados, sem o header */
	int			free;
	struct kmalloc_hdr	*next;	/* proximo bloco fisicamente seguinte, ou NULL */
};

static struct kmalloc_hdr *heap_head;

void
kheap_init(void)
{
	uint32_t va;

	for (va = HEAP_BASE; va < HEAP_BASE + HEAP_SIZE; va += PAGE_SIZE) {
		uint32_t pa = pmm_alloc();

		if (pa == PMM_ENOMEM)
			panic("kheap: sem pagina fisica pro heap inicial");

		if (vmm_map(va, pa, PAGE_PRESENT | PAGE_WRITE) != 0)
			panic("kheap: vmm_map falhou em 0x%x", va);
	}

	heap_head = (struct kmalloc_hdr *)HEAP_BASE;
	heap_head->magic = HDR_MAGIC;
	heap_head->size = HEAP_SIZE - sizeof(struct kmalloc_hdr);
	heap_head->free = 1;
	heap_head->next = NULL;

	klog("kheap", "%u paginas mapeadas em 0x%x-0x%x (%u KB)",
	    HEAP_PAGES, HEAP_BASE, HEAP_BASE + HEAP_SIZE, HEAP_SIZE / 1024);
}

void *
kmalloc(size_t size)
{
	struct kmalloc_hdr *b;

	if (size == 0)
		return NULL;

	size = ALIGN_UP(size, HEAP_ALIGN);

	for (b = heap_head; b != NULL; b = b->next) {
		if (!b->free || b->size < size)
			continue;

		/* sobra espaco suficiente pra valer a pena dividir o
		   bloco em vez de desperdicar tudo nele? */
		if (b->size >= size + sizeof(struct kmalloc_hdr) + HEAP_ALIGN) {
			struct kmalloc_hdr *nb = (struct kmalloc_hdr *)
			    ((char *)(b + 1) + size);

			nb->magic = HDR_MAGIC;
			nb->size = b->size - size - sizeof(struct kmalloc_hdr);
			nb->free = 1;
			nb->next = b->next;

			b->size = size;
			b->next = nb;
		}

		b->free = 0;
		return (void *)(b + 1);
	}

	return NULL;	/* heap cheio - por enquanto nao cresce sozinho */
}

void
kfree(void *ptr)
{
	struct kmalloc_hdr *b, *p, *prev;

	if (ptr == NULL)
		return;

	b = (struct kmalloc_hdr *)ptr - 1;

	if (b->magic != HDR_MAGIC)
		panic("kfree: ponteiro invalido ou heap corrompida (0x%x)",
		    (uint32_t)ptr);

	b->free = 1;

	/* funde com o proximo bloco, se tambem estiver livre */
	if (b->next != NULL && b->next->free) {
		b->size += sizeof(struct kmalloc_hdr) + b->next->size;
		b->next = b->next->next;
	}

	/* lista e simples (so "next"), entao pra fundir com o anterior
	   precisa andar do inicio ate achar quem aponta pra b */
	prev = NULL;
	for (p = heap_head; p != NULL && p != b; p = p->next)
		prev = p;

	if (prev != NULL && prev->free) {
		prev->size += sizeof(struct kmalloc_hdr) + b->size;
		prev->next = b->next;
	}
}
