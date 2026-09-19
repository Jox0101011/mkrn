/*
 * gdt_flush.S - lgdt + recarga dos segmentos
 *
 * cs nao pode ser trocado com um mov comum, entao usamos um far
 * jump pra forcar o processador a reler o descritor de codigo.
 */

	.code32

	.section .text
	.global gdt_flush
	.type gdt_flush, @function
gdt_flush:
	mov	4(%esp), %eax		/* ponteiro pro gdt_ptr */
	lgdt	(%eax)

	mov	$0x10, %ax		/* GSEL_KDATA */
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	mov	%ax, %ss

	ljmp	$0x08, $1f		/* GSEL_KCODE, recarrega cs */
1:
	ret
	.size	gdt_flush, . - gdt_flush

	.section .note.GNU-stack, "", @progbits
