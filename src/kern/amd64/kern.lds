/*
 * kern.lds - layout de memoria do kernel (higher half), fonte
 *
 * isso passa pelo preprocessador de c (regra no Makefile, gera
 * amd64/kern.ld) so pra poder incluir machine/pmap.h e usar as
 * MESMAS constantes KERNBASE/KERNPHYS que o resto do kernel usa -
 * antes esse valor (0xc0000000) estava duplicado aqui, em pmap.h, e
 * como literal no calculo de pde em boot.S; agora so existe um
 * lugar (pmap.h) e os outros dois (aqui e boot.S) incluem ele.
 *
 * o kernel executa em kernbase+ mas fisicamente esta carregado logo
 * acima de kernphys, como sempre esteve - a diferenca e o
 * ". += KERNBASE" no meio do script: daqui pra frente, cada secao
 * tem endereco virtual (vma) = kernbase + endereco fisico, mas o
 * AT(...) diz pro linker que o conteudo de verdade (lma, o que o
 * grub carrega) continua no endereco fisico baixo, sem deslocamento
 * nenhum.
 *
 * o pequeno trecho de amd64/boot.S que roda antes da paginacao
 * existir (.multiboot, .boot.text, .boot.bss) fica antes desse
 * deslocamento de proposito - ainda nao tem mmu traduzindo nada
 * nesse momento, entao esse codigo so funciona se rodar exatamente
 * no endereco onde foi carregado.
 *
 * kernel_start/kernel_end sao fisicos (cobrem o trecho de boot
 * junto com o kernel de verdade) - o pmm usa isso pra reservar a
 * regiao certa, sem se importar com o deslocamento de kernbase.
 */

#include "include/machine/kernbase.h"

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386:i386)
ENTRY(_start)

SECTIONS
{
	. = KERNPHYS;
	kernel_start = .;

	.multiboot : {
		*(.multiboot)
	}

	.boot.text : {
		*(.boot.text)
	}

	.boot.bss (NOLOAD) : {
		*(.boot.bss)
	}

	. = ALIGN(4096);
	. += KERNBASE;

	.text ALIGN(4096) : AT(ADDR(.text) - KERNBASE) {
		*(.text*)
	}

	.rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNBASE) {
		*(.rodata*)
	}

	.data ALIGN(4096) : AT(ADDR(.data) - KERNBASE) {
		*(.data*)
	}

	.bss ALIGN(4096) : AT(ADDR(.bss) - KERNBASE) {
		*(COMMON)
		*(.bss*)
	}

	. = ALIGN(4096);
	kernel_end = . - KERNBASE;
}
