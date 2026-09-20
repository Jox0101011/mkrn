/*
 * subr_thread.c - threads de kernel e escalonador cooperativo
 *
 * ainda so existe uma task (a do kernel - todas as threads
 * compartilham o mesmo address space, e por isso que sao "threads
 * de kernel" e nao processos de verdade). uma task com processo
 * proprio, address space separada, viria de um pgdir_phys diferente
 * e apareceria na lista global de tasks - a estrutura ja esta
 * pronta pra isso, so nao tem quem crie ainda.
 */

#include <stddef.h>
#include <stdint.h>

#include "amd64/include/machine/context.h"
#include "amd64/include/machine/cpufunc.h"
#include "sys/kmalloc.h"
#include "sys/log.h"
#include "sys/panic.h"
#include "sys/thread.h"

#define THREAD_STACK_SIZE	4096

struct task kernel_task;

static struct thread *current;		/* thread rodando agora, ou NULL antes do 1o swtch */
static struct thread *run_queue;	/* fila circular; entrada = proxima thread criada */

static void
thread_trampoline(void)
{
	current->entry();

	/* as threads de hoje sao for(;;) sem fim - se voltou, e bug
	   de quem escreveu a thread, nao tem pra onde essa execucao ir */
	panic("thread: a funcao da thread retornou (nao deveria)");
}

void
sched_init(void)
{
	kernel_task.pgdir_phys = rcr3();
	kernel_task.captbl = NULL;
	kernel_task.ipc = NULL;
	kernel_task.threads = NULL;
	kernel_task.next = NULL;

	current = NULL;
	run_queue = NULL;

	klog("sched", "escalonador cooperativo pronto (task do kernel, pgdir=0x%x)",
	    kernel_task.pgdir_phys);
}

struct thread *
thread_create(struct task *task, void (*entry)(void))
{
	struct thread *t;
	struct context *ctx;

	t = kmalloc(sizeof(*t));
	if (t == NULL)
		panic("thread_create: sem memoria pra struct thread");

	t->stack = kmalloc(THREAD_STACK_SIZE);
	if (t->stack == NULL)
		panic("thread_create: sem memoria pra stack");

	t->stack_size = THREAD_STACK_SIZE;
	t->entry = entry;
	t->state = THREAD_READY;
	t->task = task;

	/*
	 * monta o topo da stack pra parecer que a thread ja passou
	 * por um swtch() e esta prestes a dar ret pro trampolim - ver
	 * o comentario em amd64/switch.S. os callee-saved iniciais
	 * (edi/esi/ebx/ebp) nunca sao lidos de verdade nessa primeira
	 * vez, entao zero serve.
	 */
	ctx = (struct context *)(t->stack + THREAD_STACK_SIZE - sizeof(struct context));
	ctx->edi = 0;
	ctx->esi = 0;
	ctx->ebx = 0;
	ctx->ebp = 0;
	ctx->eip = (uint32_t)thread_trampoline;
	t->context = ctx;

	/* fila circular de escalonamento */
	if (run_queue == NULL) {
		t->next = t;
		run_queue = t;
	} else {
		t->next = run_queue->next;
		run_queue->next = t;
	}

	/* lista de threads da task dona */
	t->task_next = task->threads;
	task->threads = t;

	return t;
}

void
yield(void)
{
	struct thread *prev = current;

	if (prev == NULL || prev->next == prev)
		return;		/* nenhuma ou so uma thread - nada pra trocar */

	prev->state = THREAD_READY;
	current = prev->next;
	current->state = THREAD_RUNNING;

	swtch(&prev->context, current->context);
}

void
scheduler_start(void)
{
	struct context *unused;

	if (run_queue == NULL)
		panic("scheduler_start: nenhuma thread criada");

	current = run_queue;
	current->state = THREAD_RUNNING;

	/* primeira troca: nao tem thread "anterior" de verdade pra
	   salvar, entao o contexto salvo em unused e descartado */
	swtch(&unused, current->context);

	/* so chega aqui se todas as threads morrerem algum dia -
	   ainda nao existe essa nocao, entao isso e inalcancavel hoje */
	panic("scheduler_start: escalonador voltou (nao deveria)");
}
