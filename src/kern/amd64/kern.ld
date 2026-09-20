/*
 * kern.ld - layout de memoria do kernel
 *
 * multiboot exige que o kernel esteja abaixo dos 4gb e o grub carrega
 * cada segmento no endereco fisico indicado no elf, entao carregamos
 * em 1m, endereco convencional que fica acima da bios/video/etc.
 *
 * kernel_start/kernel_end marcam onde o binario carregado comeca e
 * termina fisicamente - o pmm usa isso pra nao entregar pagina que
 * o proprio kernel esta ocupando.
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(_start)

SECTIONS
{
	. = 1M;
	kernel_start = .;

	.multiboot : {
		*(.multiboot)
	}

	.text : {
		*(.text*)
	}

	.rodata : {
		*(.rodata*)
	}

	.data : {
		*(.data*)
	}

	.bss : {
		*(COMMON)
		*(.bss*)
	}

	. = ALIGN(4096);
	kernel_end = .;
}
