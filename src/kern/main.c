/*
 * main.c - ponto de entrada em C do kernel
 *
 * chamado por amd64/boot.S com magic/mbi_phys ainda em registradores
 * de 32 bits (estamos em modo protegido, nao em modo longo).
 */

#include <stddef.h>
#include <stdint.h>

#include "amd64/include/machine/cpufunc.h"
#include "amd64/include/machine/idt.h"
#include "amd64/include/machine/multiboot.h"
#include "amd64/include/machine/pic.h"
#include "amd64/include/machine/pit.h"
#include "amd64/include/machine/segments.h"
#include "sys/clock.h"
#include "sys/cons.h"
#include "sys/log.h"

void tsc_calibrate(void);			/* amd64/tsc.c */
void pmm_bootstrap(struct multiboot_info *mbi);	/* amd64/pmm_boot.c */

void
kmain(uint32_t magic, uint32_t mbi_phys)
{
	struct multiboot_info *mbi;

	vga_init();
	gdt_init();
	idt_init();
	pic_init();
	tsc_calibrate();
	pit_init(HZ);

	klog(NULL, "mkrn 0.1 (amd64/bios)");
	klog("gdt", "%d descritores carregados (cs=0x%x ds=0x%x)",
	    NGDT, GSEL_KCODE, GSEL_KDATA);
	klog("idt", "%d vetores de excecao instalados (0-%d)", NEXC, NEXC - 1);
	klog("pic", "8259 remapeado (irq0-15 -> vetor %d-%d), tudo mascarado",
	    IRQ_BASE, IRQ_BASE + NIRQ - 1);
	klog("timer", "pit no canal 0 a %d hz (1 tick = %d ms), irq0 desmascarada",
	    HZ, 1000 / HZ);

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

	pmm_bootstrap(mbi);

	klog(NULL, "main: inicializacao concluida");

	sti();		/* so agora comeca a receber a irq0 do timer */

idle:
	for (;;)
		__asm__ volatile("hlt");
}
