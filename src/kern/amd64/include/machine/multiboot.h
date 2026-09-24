/*
 * multiboot.h - estruturas da especificacao multiboot2
 *
 * ver https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html
 *
 * diferente do multiboot1 (uma struct fixa com bitmap de flags), a
 * info do boot loader aqui e uma sequencia de tags de tamanho
 * variavel logo depois do cabecalho de 8 bytes (total_size+reserved)
 * - cada tag comeca alinhada em 8 bytes, e termina com uma tag tipo
 * MB2_TAG_END. mb2_find_tag() varre essa lista.
 */

#ifndef _MACHINE_MULTIBOOT_H_
#define _MACHINE_MULTIBOOT_H_

#include "../../../sys/types.h"


/* valor de eax quando o bootloader passa controle pro kernel */
#define MB2_BOOTLOADER_MAGIC	0x36d76289

/* tipos de tag da info de boot (nao confundir com as tags do
   cabecalho, que sao outro numero pros mesmos nomes) */
#define MB2_TAG_END		0
#define MB2_TAG_CMDLINE		1
#define MB2_TAG_BOOT_LOADER_NAME 2
#define MB2_TAG_MODULE		3
#define MB2_TAG_BASIC_MEMINFO	4
#define MB2_TAG_MMAP		6
#define MB2_TAG_FRAMEBUFFER	8

struct mb2_info {
	uint32_t	total_size;	/* tudo, incluindo essa struct e a tag final */
	uint32_t	reserved;
	/* tags comecam aqui (mb2_info + 1), 8 bytes alinhado */
};

struct mb2_tag {
	uint32_t	type;
	uint32_t	size;		/* inclui esse cabecalho, sem padding */
};

struct mb2_tag_string {
	uint32_t	type, size;
	char		string[];
};

struct mb2_tag_module {
	uint32_t	type, size;
	uint32_t	mod_start;
	uint32_t	mod_end;
	char		cmdline[];
};

struct mb2_tag_meminfo {
	uint32_t	type, size;
	uint32_t	mem_lower;	/* em kb */
	uint32_t	mem_upper;
};

/* tipos de mb2_mmap_entry.type */
#define MB2_MEMORY_AVAILABLE		1
#define MB2_MEMORY_RESERVED		2
#define MB2_MEMORY_ACPI_RECLAIMABLE	3
#define MB2_MEMORY_NVS			4
#define MB2_MEMORY_BADRAM		5

struct mb2_mmap_entry {
	uint64_t	addr;
	uint64_t	len;
	uint32_t	type;
	uint32_t	reserved;
} __attribute__((packed));

struct mb2_tag_mmap {
	uint32_t	type, size;
	uint32_t	entry_size;	/* pode crescer no futuro - anda por isso, nao por sizeof(entry) */
	uint32_t	entry_version;
	struct mb2_mmap_entry entries[];
};

/* framebuffer_type: 0 = paleta indexada, 1 = rgb direto, 2 = texto ega.
   so suportamos rgb direto (amd64/fb.c) - os outros dois caem pro
   vga de texto, como se a tag nem tivesse vindo */
#define MB2_FB_TYPE_INDEXED	0
#define MB2_FB_TYPE_RGB		1
#define MB2_FB_TYPE_EGA_TEXT	2

struct mb2_tag_framebuffer {
	uint32_t	type, size;
	uint64_t	addr;
	uint32_t	pitch;
	uint32_t	width, height;
	uint8_t		bpp;
	uint8_t		fb_type;
	uint8_t		reserved[2];
	/* validos so quando fb_type == MB2_FB_TYPE_RGB; pra indexado
	   viria uma paleta aqui, e pra texto ega nao vem nada */
	uint8_t		red_pos, red_size;
	uint8_t		green_pos, green_size;
	uint8_t		blue_pos, blue_size;
} __attribute__((packed));

/* acha a primeira tag do tipo pedido, ou NULL se o bootloader nao
   mandou - varredura linear simples, a lista de tags e pequena (uma
   duzia no maximo) e isso so roda uma vez no boot */
static __inline struct mb2_tag *
mb2_find_tag(struct mb2_info *mbi, uint32_t type)
{
	struct mb2_tag *tag;

	for (tag = (struct mb2_tag *)(mbi + 1); tag->type != MB2_TAG_END;
	    tag = (struct mb2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7)))
		if (tag->type == type)
			return tag;

	return NULL;
}

void pmm_bootstrap(struct mb2_info *mbi);	/* amd64/pmm_boot.c */

#endif /* !_MACHINE_MULTIBOOT_H_ */
