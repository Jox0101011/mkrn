/*
 * context.h - contexto de troca de thread (amd64, modo protegido)
 *
 * so os registradores "callee-saved" da convencao cdecl (edi, esi,
 * ebx, ebp) mais eip - os "caller-saved" (eax, ecx, edx) nao
 * precisam ser salvos porque quem chama swtch() ja sabe que eles
 * nao sobrevivem a uma chamada de funcao, convencao normal de C.
 *
 * a ordem dos campos importa: bate exatamente com a ordem que
 * amd64/switch.S empilha/desempilha.
 */

#ifndef _MACHINE_CONTEXT_H_
#define _MACHINE_CONTEXT_H_

#include <stdint.h>

struct context {
	uint32_t	edi;
	uint32_t	esi;
	uint32_t	ebx;
	uint32_t	ebp;
	uint32_t	eip;
};

/*
 * salva os callee-saved da thread atual, guarda o esp resultante em
 * *old, troca pra stack de "new" e desempilha os dela. o ret final
 * "retorna" pro eip que estava no topo da stack de new - tanto pode
 * ser onde ela foi suspensa da ultima vez (troca normal) quanto o
 * trampolim de entrada (primeira vez que ela roda, ver
 * thread_create() em subr_thread.c).
 */
void swtch(struct context **old, struct context *new);

#endif /* !_MACHINE_CONTEXT_H_ */
