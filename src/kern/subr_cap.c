/*
 * subr_cap.c - tabela de capabilities (ver sys/cap.h pro desenho geral)
 *
 * slots[] e um array simples que so cresce (nunca encolhe de volta -
 * uma tabela de capabilities nao fica indo e vindo de tamanho, e
 * encolher exigiria mexer nos handles ja distribuidos). cap_insert()
 * reaproveita buraco deixado por cap_delete() antes de crescer; a
 * busca por um buraco e linear, o suficiente pra uma tabela do
 * tamanho que uma task tem hoje (nao e feito pra milhares de caps).
 *
 * preempcao existe (subr_thread.c: scheduler_tick()), e mais de uma
 * thread da MESMA task pode mexer na captbl dela ao mesmo tempo -
 * por isso cli_save()/sti_restore() em volta de toda operacao que
 * toca slots[]/cap, do mesmo jeito que kmalloc() protege o heap.
 */

#include "sys/types.h"
#include "amd64/include/machine/cpufunc.h"
#include "sys/cap.h"
#include "sys/kmalloc.h"

#define CAPTBL_INITIAL	16	/* slots na primeira vez que precisa crescer */

struct captbl {
	struct capslot	*slots;
	uint32_t	cap;	/* quantos slots cabem hoje em slots[] (nao confundir com "capability") */
};

struct captbl *
captbl_create(void)
{
	struct captbl *t = kmalloc(sizeof(*t));

	if (t == NULL)
		return NULL;

	t->slots = NULL;
	t->cap = 0;

	return t;
}

void
captbl_destroy(struct captbl *t)
{
	if (t == NULL)
		return;

	kfree(t->slots);
	kfree(t);
}

/* dobra a tabela (ou comeca com CAPTBL_INITIAL, se ainda vazia) -
   chamada so de dentro de cap_insert(), ja com interrupcoes
   desligadas */
static int
captbl_grow(struct captbl *t)
{
	uint32_t newcap = (t->cap == 0) ? CAPTBL_INITIAL : t->cap * 2;
	struct capslot *newslots = kmalloc(newcap * sizeof(struct capslot));
	uint32_t i;

	if (newslots == NULL)
		return -1;

	for (i = 0; i < t->cap; i++)
		newslots[i] = t->slots[i];
	for (; i < newcap; i++) {
		newslots[i].kind = CAP_NONE;
		newslots[i].rights = 0;
		newslots[i].epoch = 0;
		newslots[i].obj = NULL;
	}

	kfree(t->slots);
	t->slots = newslots;
	t->cap = newcap;

	return 0;
}

uint32_t
cap_insert(struct captbl *t, uint32_t kind, uint32_t rights, void *obj)
{
	uint32_t flags, i, handle;

	if (t == NULL || kind == CAP_NONE)
		return CAP_INVALID;

	flags = cli_save();

	for (i = 0; i < t->cap; i++)
		if (t->slots[i].kind == CAP_NONE)
			break;

	if (i == t->cap && captbl_grow(t) != 0) {
		sti_restore(flags);
		return CAP_INVALID;
	}

	handle = i;
	t->slots[handle].kind = kind;
	t->slots[handle].rights = rights;
	t->slots[handle].obj = obj;
	/* epoch NAO mexe aqui - so cap_delete() avanca (ver sys/cap.h) */

	sti_restore(flags);

	return handle;
}

struct capslot *
cap_lookup(struct captbl *t, uint32_t handle, uint32_t kind)
{
	struct capslot *s;
	uint32_t flags;

	if (t == NULL || handle >= t->cap)
		return NULL;

	flags = cli_save();
	s = (t->slots[handle].kind == kind) ? &t->slots[handle] : NULL;
	sti_restore(flags);

	return s;
}

int
cap_delete(struct captbl *t, uint32_t handle)
{
	uint32_t flags;

	if (t == NULL || handle >= t->cap)
		return -1;

	flags = cli_save();

	if (t->slots[handle].kind == CAP_NONE) {
		sti_restore(flags);
		return -1;
	}

	t->slots[handle].kind = CAP_NONE;
	t->slots[handle].rights = 0;
	t->slots[handle].obj = NULL;
	t->slots[handle].epoch++;	/* proximo handle reciclado nesse slot conta com outro epoch */

	sti_restore(flags);

	return 0;
}
