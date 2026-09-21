/*
 * pmm_boot.c - traduz o memory map do multiboot pro pmm
 *
 * layout resultante na ram (a partir de 1m, onde o kernel carrega):
 *
 *   [reservado]     - abaixo de 1m: bios/video/etc (fora do kernel;
 *                      reservado por inteiro de proposito, mesmo se
 *                      o mmap disser que parte esta disponivel)
 *   [kernel]        - .text/.rodata/.data/.bss, inclui a stack inicial
 *   [mbi/mmap/...]  - se o grub deixou isso logo depois do kernel
 *   [bitmap]        - depois de tudo isso, tamanho = ram/4096/8
 *   [livre]         - o resto, controlado pelo bitmap
 *   [reservado]     - o que o firmware marcou como nao disponivel
 *
 * duas passadas pelo mmap: a primeira so descobre ate onde vai a ram
 * disponivel, pra saber o tamanho do bitmap; so depois de pmm_init()
 * a segunda passada de fato libera as faixas disponiveis. por cima
 * disso, reservamos de volta kernel/bitmap/mbi/mmap/cmdline/modulos -
 * o bootloader nao tem como saber que essas areas estao ocupadas,
 * pra ele e ram disponivel comum.
 *
 * importante: o grub costuma colocar mbi, o array do mmap, as
 * strings de cmdline/nome do bootloader e a tabela de modulos logo
 * depois de onde o kernel foi carregado - exatamente onde a versao
 * antiga desse arquivo colocava o bitmap! o pmm_init() zera (com
 * 0xff) a regiao do bitmap assim que e chamado, entao se o bitmap
 * caisse em cima de algo disso, essa segunda leitura do mmap (pra
 * liberar as faixas disponiveis) ia ler lixo. por isso o bitmap so
 * e posicionado depois de boot_data_end(), que ja conta com tudo
 * isso - nao so kernel_end.
 *
 * page tables entram na lista de reservas quando o vmm mapear algo
 * fora do range baixo que o proprio pmap.c cobre; a stack inicial ja
 * mora dentro do kernel (amd64/boot.S coloca ela no .bss), entao ja
 * esta coberta.
 */

#include "../sys/types.h"
#include "include/machine/multiboot.h"
#include "../sys/libkern.h"
#include "../sys/log.h"
#include "../sys/panic.h"
#include "../sys/pmm.h"

extern char kernel_start[];	/* amd64/kern.lds */
extern char kernel_end[];

static __inline uint32_t
align_up(uint32_t addr, uint32_t align)
{
	return (addr + align - 1) & ~(align - 1);
}

/*
 * maior endereco (exclusive), em uint64_t o tempo todo: com ram
 * perto de 4g, "max_addr + PAGE_SIZE - 1" em 32 bits estoura e da
 * total_pages=0 (e panic logo depois, achando que nao tem ram
 * nenhuma) - por isso isso tudo e uint64_t ate a hora de truncar de
 * verdade, no final de pmm_bootstrap().
 */
static uint64_t
mmap_max_available(struct multiboot_info *mbi)
{
	struct multiboot_mmap_entry *ent;
	uint32_t off;
	uint64_t max_addr, end;

	if (!(mbi->flags & MULTIBOOT_INFO_MEM_MAP))
		return 0x100000ULL + (uint64_t)mbi->mem_upper * 1024;

	max_addr = 0;
	for (off = 0; off < mbi->mmap_length; off += ent->size + sizeof(ent->size)) {
		ent = (struct multiboot_mmap_entry *)(uintptr_t)(mbi->mmap_addr + off);
		if (ent->type != MULTIBOOT_MEMORY_AVAILABLE)
			continue;

		end = ent->addr + ent->len;
		if (end > 0x100000000ULL)	/* 32 bits sem pae: nao vai alem disso */
			end = 0x100000000ULL;
		if (end > max_addr)
			max_addr = end;
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

/*
 * maior endereco fisico tocado por qualquer coisa que o grub tenha
 * deixado pro kernel ler (a propria mbi, o array do mmap, as
 * strings, a tabela de modulos e os modulos em si). o bitmap so pode
 * comecar depois disso - ver o comentario grande no topo do arquivo.
 */
static uint32_t
boot_data_end(struct multiboot_info *mbi)
{
	uint32_t end = (uint32_t)kernel_end;
	uint32_t v;

	v = (uint32_t)mbi + sizeof(*mbi);
	if (v > end)
		end = v;

	if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
		v = mbi->mmap_addr + mbi->mmap_length;
		if (v > end)
			end = v;
	}

	if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) && mbi->cmdline != 0) {
		v = mbi->cmdline + (uint32_t)strlen((char *)(uintptr_t)mbi->cmdline) + 1;
		if (v > end)
			end = v;
	}

	if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
		v = mbi->boot_loader_name +
		    (uint32_t)strlen((char *)(uintptr_t)mbi->boot_loader_name) + 1;
		if (v > end)
			end = v;
	}

	if (mbi->flags & MULTIBOOT_INFO_MODS) {
		struct multiboot_mod_entry *mod =
		    (struct multiboot_mod_entry *)(uintptr_t)mbi->mods_addr;
		uint32_t i;

		v = mbi->mods_addr + mbi->mods_count * sizeof(*mod);
		if (v > end)
			end = v;

		for (i = 0; i < mbi->mods_count; i++)
			if (mod[i].mod_end > end)
				end = mod[i].mod_end;
	}

	return end;
}

/* reserva mbi, o array do mmap, as strings e cada modulo individual -
   um microkernel carrega os servidores exatamente assim */
static void
reserve_boot_data(struct multiboot_info *mbi)
{
	pmm_reserve((uint32_t)mbi, sizeof(*mbi));

	if (mbi->flags & MULTIBOOT_INFO_MEM_MAP)
		pmm_reserve(mbi->mmap_addr, mbi->mmap_length);

	if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) && mbi->cmdline != 0)
		pmm_reserve(mbi->cmdline,
		    strlen((char *)(uintptr_t)mbi->cmdline) + 1);

	if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		pmm_reserve(mbi->boot_loader_name,
		    strlen((char *)(uintptr_t)mbi->boot_loader_name) + 1);

	if (mbi->flags & MULTIBOOT_INFO_MODS) {
		struct multiboot_mod_entry *mod =
		    (struct multiboot_mod_entry *)(uintptr_t)mbi->mods_addr;
		uint32_t i;

		pmm_reserve(mbi->mods_addr, mbi->mods_count * sizeof(*mod));

		for (i = 0; i < mbi->mods_count; i++) {
			pmm_reserve(mod[i].mod_start,
			    mod[i].mod_end - mod[i].mod_start);
			klog("boot", "modulo %u: 0x%x-0x%x", i,
			    mod[i].mod_start, mod[i].mod_end);
		}
	}
}

void
pmm_bootstrap(struct multiboot_info *mbi)
{
	uint64_t max_addr, total_pages64;
	uint32_t bitmap_phys, bitmap_bytes, bitmap_pages;
	unsigned long total_pages;

	max_addr = mmap_max_available(mbi);
	total_pages64 = (max_addr + PAGE_SIZE - 1) / PAGE_SIZE;
	total_pages = (unsigned long)total_pages64;	/* cabe folgado: no maximo 4g/4096 */

	bitmap_bytes = (uint32_t)((total_pages + 7) / 8);
	bitmap_pages = (bitmap_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

	/* bitmap depois de tudo que o grub possa ter deixado por perto
	   do kernel - nao so depois do kernel em si */
	bitmap_phys = align_up(boot_data_end(mbi), PAGE_SIZE);

	if ((uint64_t)bitmap_phys + (uint64_t)bitmap_pages * PAGE_SIZE > max_addr)
		panic("pmm: ram insuficiente pro bitmap (%u paginas)", bitmap_pages);

	pmm_init((void *)bitmap_phys, total_pages);

	mmap_free_available(mbi);

	pmm_reserve(0, 0x100000);		/* abaixo de 1m: nunca aloca daqui */
	pmm_reserve((uint32_t)kernel_start,
	    (uint32_t)kernel_end - (uint32_t)kernel_start);	/* kernel (com a stack) */
	pmm_reserve(bitmap_phys, (uint64_t)bitmap_pages * PAGE_SIZE); /* o bitmap */
	reserve_boot_data(mbi);

	klog("pmm", "kernel   0x%x-0x%x", (uint32_t)kernel_start, (uint32_t)kernel_end);
	klog("pmm", "bitmap   0x%x-0x%x (%u paginas)",
	    bitmap_phys, bitmap_phys + bitmap_pages * PAGE_SIZE, bitmap_pages);
	klog("pmm", "%lu paginas (%lu MB), %lu livres (%lu MB)",
	    pmm_npages(), pmm_npages() / 256,
	    pmm_nfree(), pmm_nfree() / 256);
}
