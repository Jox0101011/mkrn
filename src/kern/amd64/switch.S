/*
 * switch.S - troca de contexto entre threads
 *
 * void swtch(struct context **old, struct context *new);
 *
 * salva eflags + callee-saved da thread atual, guarda o esp
 * resultante em *old, troca esp pro da thread nova, desfaz o mesmo
 * pra ela, e da ret. esse ret "retorna" pro eip que estava no topo
 * da stack da thread nova - normal em toda troca depois da primeira
 * (ela literalmente estava parada ali dentro de um swtch() anterior,
 * cooperativo ou puxado pelo timer - ver amd64/pit.c), ou o
 * trampolim de entrada na primeira vez que a thread roda (a stack
 * dela foi montada a mao pra parecer que ja passou por aqui uma
 * vez, ver thread_create()).
 *
 * pushf/popf existem so por causa do bit IF - ver o comentario grande
 * em machine/context.h sobre o porque disso importar quando a troca
 * vem de dentro do handler do timer.
 */

	.code32
	.section .text
	.global swtch
	.type swtch, @function
swtch:
	mov	4(%esp), %eax		/* struct context **old */
	mov	8(%esp), %edx		/* struct context *new */

	pushf
	push	%ebp
	push	%ebx
	push	%esi
	push	%edi

	mov	%esp, (%eax)		/* *old = esp atual */
	mov	%edx, %esp		/* troca de stack pra thread nova */

	pop	%edi
	pop	%esi
	pop	%ebx
	pop	%ebp
	popf

	ret
	.size	swtch, . - swtch

	.section .note.GNU-stack, "", @progbits
