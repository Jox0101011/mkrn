/*
 * cap.h - tabela de capabilities (captbl)
 *
 * todo recurso protegido do kernel (endpoint, VMO, IRQ, outra task,
 * etc) e referenciado de fora por um handle (uint32_t) - nunca por
 * ponteiro de kernel direto. cada task tem sua propria captbl
 * (sys/thread.h: task->captbl); o handle so faz sentido dentro dela,
 * igual um fd so faz sentido dentro do processo dono.
 *
 *   handle (uint32_t)
 *        |
 *        v
 *   captbl->slots[handle]  ->  struct capslot { kind, rights, epoch, obj }
 *                                                                |
 *                                                                v
 *                                                          objeto de kernel de verdade
 *
 * cap_lookup() e o UNICO jeito de chegar no objeto de verdade a
 * partir de um handle - e ele que confere o kind esperado, entao
 * quem chama nunca precisa (nem pode) desreferenciar slot->obj sem
 * passar por essa checagem antes.
 *
 * epoch existe pra detectar handle velho apontando pra um slot
 * reciclado: cap_delete() avanca o epoch do slot antes de devolve-lo
 * pro pool de livres. ninguem ainda guarda um epoch fora da tabela
 * pra comparar depois (isso e coisa de quando ipc passar handle de
 * task pra task) - por enquanto so existe o suficiente pra esse
 * campo nao ficar inerte.
 *
 * so os kinds que ja tem um objeto de kernel de verdade por tras
 * (CAP_TASK, CAP_THREAD) sao inseridos por algo hoje; o resto do
 * enum existe porque a lista inteira de recursos precisa estar
 * definida antes de endpoint/VMO/etc chegarem - inserir uma cap
 * desses tipos ainda funciona (a tabela nao sabe nem quer saber o
 * que e obj), so nao tem quem faca isso de verdade ainda.
 */

#ifndef _SYS_CAP_H_
#define _SYS_CAP_H_

#include "types.h"


#define CAP_INVALID	0xffffffffu	/* nunca um handle valido - a tabela nunca cresce tao grande */

enum cap_kind {
	CAP_NONE = 0,		/* slot vazio - nao e um kind que se insere */
	CAP_ENDPOINT,
	CAP_CHANNEL,
	CAP_NOTIFICATION,
	CAP_VMO,
	CAP_WAITSET,
	CAP_IRQ,
	CAP_MMIO,
	CAP_IOPORT,
	CAP_TASK,
	CAP_THREAD,
	CAP_DEVICE,
};

/*
 * rights e uma bitmask opaca pra esta camada: cada kind define o
 * que os bits dela significam (ler/escrever um VMO, enviar/receber
 * num endpoint, etc) quando essas operacoes existirem. cap_insert()
 * so guarda o valor, cap_lookup() so devolve - a validacao de "essa
 * operacao especifica e permitida" e de quem move o objeto de
 * verdade, nunca da captbl.
 */
struct capslot {
	uint32_t	kind;
	uint32_t	rights;
	uint32_t	epoch;
	void		*obj;
};

struct captbl;		/* opaca - layout so importa pra subr_cap.c */

/*
 * cap_lookup() devolve um ponteiro PRA DENTRO de slots[] - valido so
 * ate a proxima chamada de cap_insert() que faca a tabela crescer
 * (crescer troca o array inteiro de lugar, ver captbl_grow() em
 * subr_cap.c). pra uso pontual (ler kind/rights/obj e pronto) isso
 * nunca e problema; so nao guarde esse ponteiro atravessando um
 * yield() ou qualquer coisa que possa rodar outra thread da mesma
 * task - faca outro cap_lookup() depois em vez disso.
 */
struct captbl *captbl_create(void);
void captbl_destroy(struct captbl *t);

uint32_t cap_insert(struct captbl *t, uint32_t kind, uint32_t rights, void *obj);
struct capslot *cap_lookup(struct captbl *t, uint32_t handle, uint32_t kind);
int cap_delete(struct captbl *t, uint32_t handle);

#endif /* !_SYS_CAP_H_ */
