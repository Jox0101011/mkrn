/*
 * idt_stubs.S - stubs de isr pras excecoes da cpu (vetores 0-31)
 *
 * cada excecao pula pro isr_common_stub, que monta um trapframe e
 * chama trap_handler() em C. algumas excecoes empilham um error
 * code sozinhas (cpu faz isso antes de entrar no gate); as que nao
 * empilham ganham um zero nosso, pra manter o trapframe do mesmo
 * jeito pra qualquer vetor.
 *
 * o gate e do tipo interrupt gate (idt.c), entao a cpu ja desliga
 * IF ao entrar e o iret restaura de volta - nao precisa de cli/sti
 * manual aqui.
 */

	.code32

.macro ISR_NOERR num
	.global isr\num
	.type isr\num, @function
isr\num:
	push	$0
	push	$\num
	jmp	isr_common_stub
	.size	isr\num, . - isr\num
.endm

.macro ISR_ERR num
	.global isr\num
	.type isr\num, @function
isr\num:
	push	$\num
	jmp	isr_common_stub
	.size	isr\num, . - isr\num
.endm

	.section .text

	ISR_NOERR 0	/* divide error */
	ISR_NOERR 1	/* debug */
	ISR_NOERR 2	/* nmi */
	ISR_NOERR 3	/* breakpoint */
	ISR_NOERR 4	/* overflow */
	ISR_NOERR 5	/* bound range exceeded */
	ISR_NOERR 6	/* invalid opcode */
	ISR_NOERR 7	/* device not available */
	ISR_ERR   8	/* double fault */
	ISR_NOERR 9	/* coprocessor segment overrun (legado) */
	ISR_ERR   10	/* invalid tss */
	ISR_ERR   11	/* segment not present */
	ISR_ERR   12	/* stack-segment fault */
	ISR_ERR   13	/* general protection */
	ISR_ERR   14	/* page fault */
	ISR_NOERR 15	/* reservado */
	ISR_NOERR 16	/* x87 fp exception */
	ISR_ERR   17	/* alignment check */
	ISR_NOERR 18	/* machine check */
	ISR_NOERR 19	/* simd fp exception */
	ISR_NOERR 20	/* virtualization exception */
	ISR_ERR   21	/* control protection exception */
	ISR_NOERR 22	/* reservado */
	ISR_NOERR 23	/* reservado */
	ISR_NOERR 24	/* reservado */
	ISR_NOERR 25	/* reservado */
	ISR_NOERR 26	/* reservado */
	ISR_NOERR 27	/* reservado */
	ISR_NOERR 28	/* reservado */
	ISR_NOERR 29	/* reservado */
	ISR_NOERR 30	/* reservado */
	ISR_NOERR 31	/* reservado */

isr_common_stub:
	pusha
	push	%esp		/* ponteiro pro trapframe, arg de trap_handler */
	call	trap_handler
	add	$4, %esp
	popa
	add	$8, %esp	/* descarta vector + err empilhados pelo isrN */
	iret
	.size	isr_common_stub, . - isr_common_stub

	.section .rodata
	.align 4
	.global isr_stub_table
isr_stub_table:
	.long	isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
	.long	isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
	.long	isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
	.long	isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

	.section .note.GNU-stack, "", @progbits
