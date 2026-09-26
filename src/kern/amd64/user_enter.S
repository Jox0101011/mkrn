/*
 * user_enter.S - primeira entrada de uma thread em ring3
 *
 * void user_enter(uint32_t eip, uint32_t esp) - nunca retorna: monta
 * o frame que o iret espera quando troca de privilegio (ss, esp,
 * eflags, cs, eip, nessa ordem empilhado de tras pra frente) e salta.
 * chamado uma unica vez, pelo trampolim (subr_thread.c), ainda em
 * ring0 e na stack de KERNEL da thread.
 *
 * ds/es/fs/gs precisam apontar pro selector de dado de usuario ANTES
 * do iret: com cpl=3 depois do salto, qualquer acesso a memoria por
 * um desses segmentos com rpl/dpl=0 (o valor que eles tem agora, de
 * kernel) da #gp na hora. ss nao da pra trocar direto com um mov
 * aqui (cpl atual=0 != rpl=3 do selector) - por isso ela so e
 * ajustada pelo proprio iret, empilhada como parte do frame.
 *
 * 0x1b/0x23 sao GSEL_UCODE/GSEL_UDATA (machine/segments.h) escritos
 * na mao: um .S so pode incluir header com #define puro (ver o
 * comentario em machine/kernbase.h) - segments.h tem struct no meio
 * e quebraria o assembler. gdt_flush.S ja faz a mesma coisa com
 * GSEL_KCODE/GSEL_KDATA.
 */

	.code32
	.section .text
	.global user_enter
	.type user_enter, @function
user_enter:
	mov	4(%esp), %eax		/* eip de usuario (1o argumento) */
	mov	8(%esp), %edx		/* esp de usuario (2o argumento) */

	mov	$0x23, %cx		/* GSEL_UDATA */
	mov	%cx, %ds
	mov	%cx, %es
	mov	%cx, %fs
	mov	%cx, %gs

	push	$0x23			/* ss (GSEL_UDATA) */
	push	%edx			/* esp */
	push	$0x202			/* eflags: if ligado */
	push	$0x1b			/* cs (GSEL_UCODE) */
	push	%eax			/* eip */
	iret
	.size	user_enter, . - user_enter

	.section .note.GNU-stack, "", @progbits
