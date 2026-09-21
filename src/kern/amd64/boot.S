/*
 * boot.S - entry point multiboot, bootstrap de paginacao, salto pro
 * higher half
 *
 * o grub carrega o kernel e pula pra _start em modo protegido de 32
 * bits, com eax = magic multiboot, ebx = ponteiro fisico pra struct
 * multiboot_info, e paginacao DESLIGADA. nesse momento o codigo
 * ainda precisa rodar em endereco fisico baixo (~1m), porque nao
 * tem mmu traduzindo nada ainda.
 *
 * como o resto do kernel e linkado pra rodar em kernbase+ (ver
 * amd64/kern.lds e machine/kernbase.h), _start e a montagem da
 * tabela temporaria ficam numa secao separada (.boot.text/.boot.bss) que o linker mantem no
 * endereco fisico, sem o deslocamento de kernbase - e o unico jeito
 * do processador conseguir buscar essas instrucoes antes da mmu
 * entrar em cena.
 *
 * a tabela temporaria mapeia os primeiros 4m de duas formas ao mesmo
 * tempo: identidade (0-4m -> 0-4m, pra nao perder o chao debaixo dos
 * pes assim que a paginacao liga) e higher half (kernbase-kernbase+4m
 * -> 0-4m, pra onde o resto do kernel foi linkado). com as duas
 * validas ao mesmo tempo, um simples "jmp higher_half_entry" (que o
 * linker resolveu pro endereco alto) troca de contexto sem drama -
 * o deslocamento relativo do jmp da conta de tudo sozinho.
 *
 * essa tabela e so um trampolim; amd64/pmap.c monta a de verdade
 * (cobrindo toda a ram) e troca o cr3 assim que o pmm estiver de pe.
 */

	.code32

#include "include/machine/kernbase.h"

/* cabecalho multiboot 1: precisa estar nos primeiros 8k do arquivo
   e alinhado em 4 bytes */
	.section .multiboot, "a"
	.align 4
mb_header:
	.long	0x1badb002			/* magic */
	.long	0x00000003			/* flags: align + meminfo */
	.long	-(0x1badb002 + 0x00000003)	/* checksum */

/* ---- daqui pra baixo: endereco fisico baixo, sem kernbase ---- */

	.section .boot.bss, "aw", @nobits
	.align 16
boot_stack_bottom:
	.skip	4096			/* stack pequena, so pro trampolim */
boot_stack_top:

	.align 4096
boot_pgtable:
	.skip	4096
boot_pgdir:
	.skip	4096

	.section .boot.text, "ax"
	.global _start
	.type _start, @function
_start:
	cli
	mov	$boot_stack_top, %esp
	mov	%esp, %ebp

	/* preenche a page table temporaria: identity dos primeiros 4m
	   (1024 entradas de 4k cada). eax/ebx (magic/mbi) ficam fora
	   disso o tempo todo. */
	mov	$boot_pgtable, %edi
	xor	%ecx, %ecx
1:
	mov	%ecx, %edx
	shl	$12, %edx
	or	$0x03, %edx		/* present | rw */
	mov	%edx, (%edi, %ecx, 4)
	inc	%ecx
	cmp	$1024, %ecx
	jne	1b

	/* pde 0: identity (0-4m -> 0-4m) */
	mov	$boot_pgtable, %edx
	or	$0x03, %edx
	mov	$boot_pgdir, %edi
	mov	%edx, (%edi)

	/* pde de kernbase (kernbase>>22): higher half, mesma tabela */
	mov	%edx, ((KERNBASE >> 22) * 4)(%edi)

	/* de agora em diante nao pode mais tocar em eax (tem o magic
	   do multiboot) nem em ebx (mbi_phys) - usa edx pro resto */
	mov	$boot_pgdir, %edx
	mov	%edx, %cr3

	mov	%cr0, %edx
	or	$0x80000000, %edx	/* cr0.pg */
	mov	%edx, %cr0

	/* eax/ebx ainda tem magic/mbi_phys intactos. o simbolo abaixo
	   foi resolvido pelo linker no higher half (kern.ld) - com as
	   duas traducoes validas agora, o salto relativo cai certinho
	   no endereco alto sozinho */
	jmp	higher_half_entry
	.size	_start, . - _start

/* ---- daqui pra baixo: endereco normal, kernbase + fisico ---- */

	.section .bss
	.align 16
stack_bottom:
	.skip	16384
stack_top:

	.section .text
	.global higher_half_entry
	.type higher_half_entry, @function
higher_half_entry:
	mov	$stack_top, %esp	/* troca pra stack de verdade, ja no higher half */
	mov	%esp, %ebp

	push	%ebx			/* mbi_phys: 2o argumento de kmain() */
	push	%eax			/* magic: 1o argumento de kmain() */
	call	kmain

	cli
1:	hlt
	jmp	1b
	.size	higher_half_entry, . - higher_half_entry

/* marca a stack como nao-executavel, silencia warning do linker */
	.section .note.GNU-stack, "", @progbits
