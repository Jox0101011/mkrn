/*
 * main.c - ponto de entrada em C do kernel
 *
 * chamado por amd64/boot.S com magic/mbi_phys ainda em registradores
 * de 32 bits (estamos em modo protegido, nao em modo longo).
 */

#include <stddef.h>
#include <stdint.h>

#include "amd64/include/machine/multiboot.h"
#include "sys/clock.h"
#include "sys/cons.h"
#include "sys/log.h"

void tsc_calibrate(void);	/* amd64/tsc.c */

void
kmain(uint32_t magic, uint32_t mbi_phys)
{
	struct multiboot_info *mbi;

	vga_init();
	tsc_calibrate();

	klog(NULL, "mkrn 0.1 (amd64/bios)");

	if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
		klog("boot", "magic multiboot invalido: 0x%x", magic);
		goto idle;
	}

	mbi = (struct multiboot_info *)(uintptr_t)mbi_phys;

	if (mbi->flags & MULTIBOOT_INFO_MEMORY)
		klog("boot", "memoria: %uK lower, %uK upper",
		    mbi->mem_lower, mbi->mem_upper);

	if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		klog("boot", "bootloader: %s",
		    (char *)(uintptr_t)mbi->boot_loader_name);

	if ((mbi->flags & MULTIBOOT_INFO_CMDLINE) && mbi->cmdline != 0)
		klog("boot", "cmdline: %s", (char *)(uintptr_t)mbi->cmdline);

	klog(NULL, "main: inicializacao concluida");

idle:
	for (;;)
		__asm__ volatile("hlt");
}
