/*
 * pmm_boot.c - traduz o memory map do multiboot pro pmm
 *
 * layout resultante na ram (a partir de 1m, onde o kernel carrega):
 *
 *   [reservado]     - abaixo de 1m: bios/video/etc (fora do kernel)
 *   [kernel]        - .text/.rodata/.data/.bss, inclui a stack inicial
 *   [bitmap]        - logo depois do kernel, tamanho = ram/4096/8
 *   [livre]         - o resto, controlado pelo bitmap
 *   [reservado]     - o que o firmware marcou como nao disponivel
 *
 * duas passadas pelo mmap: a primeira so descobre ate onde vai a ram
 * disponivel (pra saber o tamanho do bitmap e onde ele cabe); so
 * depois de pmm_init() a segunda passada de fato libera as faixas
 * disponiveis, e por cima disso reservamos de volta kernel/bitmap/mbi
 * - o bootloader nao tem como saber que essas areas estao ocupadas,
 * pra ele e ram disponivel comum. page tables entram nessa lista
 * quando o vmm existir; a stack inicial ja mora dentro do kernel
 * (amd64/boot.S coloca ela no .bss), entao ja esta coberta.
 */

#include <stddef.h>
#include <stdint.h>

#include "include/machine/multiboot.h"
#include "../sys/log.h"
#include "../sys/panic.h"
#include "../sys/pmm.h"

extern char kernel_start[];	/* amd64/kern.ld */
extern char kernel_end[];

static __inline uint32_t
align_up(uint32_t addr, uint32_t align)
{
	return (addr + align - 1) & ~(align - 1);
}

/* maior endereco (exclusive) coberto por alguma entrada disponivel */
static uint32_t
mmap_max_available(struct multiboot_info *mbi)
{
	struct multiboot_mmap_entry *ent;
	uint32_t off, max_addr;
	uint64_t end;

	if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP))
		return 0x100000 + mbi->mem_upper * 1024;

	max_addr = 0;
	for (off = 0; off < mbi->mmap_length; off += ent->size + sizeof(ent->size)) {
		ent = (struct multiboot_mmap_entry *)(uintptr_t)(mbi->mmap_addr + off);
		if (ent->type != MULTIBOOT_MEMORY_AVAILABLE)
			continue;

		end = ent->addr + ent->len;
		if (end > 0xffffffff)		/* 32 bits sem pae: nao vai alem disso */
			end = 0xffffffff;
		if ((uint32_t)end > max_addr)
			max_addr = (uint32_t)end;
	}

	return max_addr;
}

/* libera cada entrada MULTIBOOT_MEMORY_AVAILABLE do mmap */
static void
mmap_free_available(struct multiboot_info *mbi)
{
	struct multiboot_mmap_entry *ent;
	uint32_t off;

	if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP)) {
		klog("pmm", "sem memory map multiboot, usando mem_lower/upper");
		pmm_free_region(0, (uint64_t)mbi->mem_lower * 1024);
		pmm_free_region(0x100000, (uint64_t)mbi->mem_upper * 1024);
		return;
	}

	for (off = 0; off < mbi->mmap_length; off += ent->size + sizeof(ent->size)) {
		ent = (struct multiboot_mmap_entry *)(uintptr_t)(mbi->mmap_addr + off);
		if (ent->type == MULTIBOOT_MEMORY_AVAILABLE)
			pmm_free_region(ent->addr, ent->len);
	}
}

void
pmm_bootstrap(struct multiboot_info *mbi)
{
	uint32_t max_addr, bitmap_phys, bitmap_bytes, bitmap_pages;
	unsigned long total_pages;

	max_addr = mmap_max_available(mbi);
	total_pages = (max_addr + PAGE_SIZE - 1) / PAGE_SIZE;
	bitmap_bytes = (uint32_t)((total_pages + 7) / 8);
	bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

	/* bitmap logo depois do kernel; kernel_end ja vem alinhado em
	   pagina do linker script, o align_up aqui e so por garantia */
	bitmap_phys = align_up((uint32_t)kernel_end, PAGE_SIZE);

	if ((uint64_t)bitmap_phys + (uint64_t)bitmap_pages * PAGE_SIZE > max_addr)
		panic("pmm: ram insuficiente pro bitmap (%u paginas)", bitmap_pages);

	pmm_init((void *)bitmap_phys, total_pages);

	mmap_free_available(mbi);

	pmm_reserve(0, PAGE_SIZE);				/* pagina 0, sempre invalida */
	pmm_reserve((uint32_t)kernel_start,
	    (uint32_t)kernel_end - (uint32_t)kernel_start);	/* kernel (com a stack) */
	pmm_reserve(bitmap_phys, (uint64_t)bitmap_pages * PAGE_SIZE); /* o bitmap */

	/* mbi/mmap ainda estao sendo lidos agora mesmo */
	pmm_reserve((uint32_t)mbi, sizeof(*mbi));
	if (mbi->flags & MULTIBOOT_INFO_MEM_MAP)
		pmm_reserve(mbi->mmap_addr, mbi->mmap_length);

	klog("pmm", "kernel   0x%x-0x%x", (uint32_t)kernel_start, (uint32_t)kernel_end);
	klog("pmm", "bitmap   0x%x-0x%x (%u paginas)",
	    bitmap_phys, bitmap_phys + bitmap_pages * PAGE_SIZE, bitmap_pages);
	klog("pmm", "%lu paginas (%lu MB), %lu livres (%lu MB)",
	    pmm_npages(), pmm_npages() / 256,
	    pmm_nfree(), pmm_nfree() / 256);
}
