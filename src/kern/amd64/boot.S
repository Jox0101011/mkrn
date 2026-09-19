/*
 * boot.S - entry point multiboot e stack inicial
 *
 * o grub (ou outro bootloader multiboot) carrega o kernel e pula pra
 * _start em modo protegido de 32 bits, com eax contendo o magic
 * multiboot e ebx um ponteiro fisico pra struct multiboot_info.
 *
 * modo longo (64 bits) ainda nao e inicializado aqui, fica pra uma
 * proxima etapa (gdt, paginas, etc).
 */

	.code32

/* cabecalho multiboot 1: precisa estar nos primeiros 8k do arquivo
   e alinhado em 4 bytes */
	.section .multiboot, "a"
	.align 4
mb_header:
	.long	0x1badb002			/* magic */
	.long	0x00000003			/* flags: align + meminfo */
	.long	-(0x1badb002 + 0x00000003)	/* checksum */

	.section .bss
	.align 16
stack_bottom:
	.skip	16384
stack_top:

	.section .text
	.global _start
	.type _start, @function
_start:
	cli
	mov	$stack_top, %esp
	mov	%esp, %ebp

	push	%ebx		/* mbi_phys: 2o argumento de kmain() */
	push	%eax		/* magic: 1o argumento de kmain() */
	call	kmain

	cli
1:	hlt
	jmp	1b
	.size	_start, . - _start

/* marca a stack como nao-executavel, silencia warning do linker */
	.section .note.GNU-stack, "", @progbits
