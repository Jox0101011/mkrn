/*
 * switch.S - troca de contexto entre threads
 *
 * void swtch(struct context **old, struct context *new);
 *
 * empilha os callee-saved da thread atual, guarda o esp resultante
 * em *old, troca esp pro da thread nova, desempilha os dela, e da
 * ret. esse ret "retorna" pro eip que estava no topo da stack da
 * thread nova - normal em toda troca depois da primeira (ela
 * literalmente estava parada ali dentro de um swtch() anterior), ou
 * o trampolim de entrada na primeira vez que a thread roda (a stack
 * dela foi montada a mao pra parecer que ja passou por aqui uma
 * vez, ver thread_create()).
 */

	.code32
	.section .text
	.global swtch
	.type swtch, @function
swtch:
	mov	4(%esp), %eax		/* struct context **old */
	mov	8(%esp), %edx		/* struct context *new */

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

	ret
	.size	swtch, . - swtch

	.section .note.GNU-stack, "", @progbits
