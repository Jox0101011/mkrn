/*
 * multiboot.h - estruturas da especificacao multiboot 1
 *
 * ver https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
 */

#ifndef _MACHINE_MULTIBOOT_H_
#define _MACHINE_MULTIBOOT_H_

#include <stdint.h>

/* valor de eax quando o bootloader passa controle pro kernel */
#define MULTIBOOT_BOOTLOADER_MAGIC	0x2badb002

/* bits de multiboot_info.flags */
#define MULTIBOOT_INFO_MEMORY		0x00000001
#define MULTIBOOT_INFO_BOOTDEV		0x00000002
#define MULTIBOOT_INFO_CMDLINE		0x00000004
#define MULTIBOOT_INFO_MODS		0x00000008
#define MULTIBOOT_INFO_MEM_MAP		0x00000040
#define MULTIBOOT_INFO_BOOT_LOADER_NAME 0x00000200

/* struct passada em ebx, sem necessidade de packed: no i386 os campos
   de 8 bytes ja caem alinhados em 4 bytes, igual ao layout do spec */
struct multiboot_info {
	uint32_t	flags;

	uint32_t	mem_lower;
	uint32_t	mem_upper;

	uint32_t	boot_device;

	uint32_t	cmdline;

	uint32_t	mods_count;
	uint32_t	mods_addr;

	uint32_t	syms[4];

	uint32_t	mmap_length;
	uint32_t	mmap_addr;

	uint32_t	drives_length;
	uint32_t	drives_addr;

	uint32_t	config_table;

	uint32_t	boot_loader_name;

	uint32_t	apm_table;

	uint32_t	vbe_control_info;
	uint32_t	vbe_mode_info;
	uint16_t	vbe_mode;
	uint16_t	vbe_interface_seg;
	uint16_t	vbe_interface_off;
	uint16_t	vbe_interface_len;

	uint64_t	framebuffer_addr;
	uint32_t	framebuffer_pitch;
	uint32_t	framebuffer_width;
	uint32_t	framebuffer_height;
	uint8_t		framebuffer_bpp;
	uint8_t		framebuffer_type;
	uint8_t		framebuffer_color_info[6];
};

/* tipos de multiboot_mmap_entry.type */
#define MULTIBOOT_MEMORY_AVAILABLE		1
#define MULTIBOOT_MEMORY_RESERVED		2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE	3
#define MULTIBOOT_MEMORY_NVS			4
#define MULTIBOOT_MEMORY_BADRAM		5

/*
 * uma entrada do memory map (mmap_addr, quando MULTIBOOT_INFO_MEM_MAP
 * esta setado em flags). "size" e o tamanho do resto da entry (sem
 * contar o proprio campo size) - e assim que anda pro proximo: soma
 * size + sizeof(size), nao sizeof(struct), porque entries futuras
 * podem vir maiores do que essa.
 */
struct multiboot_mmap_entry {
	uint32_t	size;
	uint64_t	addr;
	uint64_t	len;
	uint32_t	type;
} __attribute__((packed));

#endif /* !_MACHINE_MULTIBOOT_H_ */
