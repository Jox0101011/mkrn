/*
 * idt_stubs.S - stubs de isr: excecoes da cpu (0-31) + irq de
 * hardware (32-47, pic remapeado - ver pic.h)
 *
 * cada um pula pro isr_common_stub, que monta um trapframe e chama
 * isr_dispatch() em C. algumas excecoes empilham um error code
 * sozinhas (cpu faz isso antes de entrar no gate); as que nao
 * empilham (incluindo todas as irqs de hardware) ganham um zero
 * nosso, pra manter o trapframe do mesmo jeito pra qualquer vetor.
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

	ISR_NOERR 32	/* irq0  - timer (pit) */
	ISR_NOERR 33	/* irq1  - teclado */
	ISR_NOERR 34	/* irq2  - cascata pra escrava, nunca dispara direto */
	ISR_NOERR 35	/* irq3  - com2 */
	ISR_NOERR 36	/* irq4  - com1 */
	ISR_NOERR 37	/* irq5  - lpt2 / som */
	ISR_NOERR 38	/* irq6  - controladora de disquete */
	ISR_NOERR 39	/* irq7  - lpt1 (sujeita a irq espuria) */
	ISR_NOERR 40	/* irq8  - rtc */
	ISR_NOERR 41	/* irq9  - livre / acpi */
	ISR_NOERR 42	/* irq10 - livre */
	ISR_NOERR 43	/* irq11 - livre */
	ISR_NOERR 44	/* irq12 - mouse ps/2 */
	ISR_NOERR 45	/* irq13 - coprocessador matematico */
	ISR_NOERR 46	/* irq14 - ata primaria */
	ISR_NOERR 47	/* irq15 - ata secundaria (sujeita a irq espuria) */

isr_common_stub:
	pusha
	push	%esp		/* ponteiro pro trapframe, arg de isr_dispatch */
	call	isr_dispatch
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
	.long	isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39
	.long	isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47

	.section .note.GNU-stack, "", @progbits
