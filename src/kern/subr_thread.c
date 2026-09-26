/*
 * subr_thread.c - threads de kernel e escalonador (round-robin,
 * cooperativo por yield() ou preemptivo por scheduler_tick())
 *
 * so existe pgdir compartilhado por enquanto (task_create() usa o
 * cr3 atual, o mesmo do kernel_task) - uma task com address space de
 * verdade separado viria de um pgdir_phys diferente, e o que
 * continua faltando pra isso e clonar/montar essa tabela nova, nao a
 * struct em si. o que ja isola thread de usuario de memoria de
 * kernel HOJE e o bit PAGE_USER por pagina (amd64/pmap.c): as
 * paginas de kernel nunca tem esse bit, entao ring3 nao enxerga nada
 * do kernel mesmo dividindo o mesmo pgdir.
 */


#include "sys/types.h"
#include "amd64/include/machine/context.h"
#include "amd64/include/machine/cpufunc.h"
#include "amd64/include/machine/segments.h"
#include "sys/kmalloc.h"
#include "sys/log.h"
#include "sys/panic.h"
#include "sys/pmm.h"
#include "sys/thread.h"
#include "sys/vmm.h"

extern void user_enter(uint32_t eip, uint32_t esp);	/* amd64/user_enter.S - nunca retorna */

#define THREAD_STACK_PAGES	1				/* 4096 bytes, como antes */
#define THREAD_STACK_SIZE	(THREAD_STACK_PAGES * PAGE_SIZE)

/* faixa de va dedicada pras stacks de thread - cada uma ganha uma
   pagina de guarda (sem mapear) logo abaixo, de proposito: um
   estouro de stack vira #pf na hora em vez de corromper em silencio
   o que estiver do lado no heap (que e o que acontecia quando a
   stack vinha de kmalloc()) */
#define THREAD_VA_BASE		0xe0000000u

struct task kernel_task;

static struct thread *current;		/* thread rodando agora, ou NULL antes do 1o swtch */
static struct thread *run_queue;	/* fila circular; entrada = proxima thread criada */
static uint32_t next_thread_va = THREAD_VA_BASE;

static uint8_t *
thread_stack_alloc(void)
{
	uint32_t guard_va, stack_va;
	unsigned i;

	guard_va = next_thread_va;		/* fica sem mapear, de proposito */
	stack_va = guard_va + PAGE_SIZE;

	for (i = 0; i < THREAD_STACK_PAGES; i++) {
		uint32_t pa = pmm_alloc();

		if (pa == PMM_ENOMEM)
			panic("thread_create: sem pagina fisica pra stack");
		if (vmm_map(stack_va + i * PAGE_SIZE, pa, PAGE_PRESENT | PAGE_WRITE) != 0)
			panic("thread_create: vmm_map falhou pra stack");
	}

	/* proxima thread comeca uma pagina depois do topo desta, pra
	   sobrar uma guarda pra ELA tambem antes da stack dela */
	next_thread_va = stack_va + THREAD_STACK_SIZE + PAGE_SIZE;

	return (uint8_t *)stack_va;
}

static void
thread_trampoline(void)
{
	if (current->user_entry != 0)
		user_enter(current->user_entry, current->user_stack);	/* nunca volta daqui */

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

	klog("sched", "escalonador round-robin pronto (task do kernel, pgdir=0x%x)",
	    kernel_task.pgdir_phys);
}

struct task *
task_create(void)
{
	struct task *t = kmalloc(sizeof(*t));

	if (t == NULL)
		panic("task_create: sem memoria pra struct task");

	t->pgdir_phys = rcr3();	/* mesmo address space de todo mundo - ver o comentario do topo */
	t->captbl = NULL;
	t->ipc = NULL;
	t->threads = NULL;
	t->next = NULL;

	return t;
}

/* tudo que thread_create() e thread_create_user() tem em comum:
   aloca a struct, a stack de KERNEL (toda thread tem uma, ver o
   comentario grande em sys/thread.h) e entra nas duas listas
   (fila de escalonamento + threads da task) */
static struct thread *
thread_new(struct task *task)
{
	struct thread *t = kmalloc(sizeof(*t));

	if (t == NULL)
		panic("thread_new: sem memoria pra struct thread");

	t->stack = thread_stack_alloc();
	t->stack_size = THREAD_STACK_SIZE;
	t->entry = NULL;
	t->user_entry = 0;
	t->user_stack = 0;
	t->state = THREAD_READY;
	t->task = task;

	if (run_queue == NULL) {
		t->next = t;
		run_queue = t;
	} else {
		t->next = run_queue->next;
		run_queue->next = t;
	}

	t->task_next = task->threads;
	task->threads = t;

	return t;
}

/*
 * monta o topo da stack pra parecer que a thread ja passou por um
 * swtch() e esta prestes a dar ret pro trampolim - ver o comentario
 * em amd64/switch.S. os callee-saved iniciais (edi/esi/ebx/ebp)
 * nunca sao lidos de verdade nessa primeira vez, entao zero serve -
 * eflags e o unico que importa de verdade: e o popf que liga
 * interrupcao a primeira vez que a thread roda.
 */
static void
thread_init_context(struct thread *t)
{
	struct context *ctx;

	ctx = (struct context *)(t->stack + THREAD_STACK_SIZE - sizeof(struct context));
	ctx->edi = 0;
	ctx->esi = 0;
	ctx->ebx = 0;
	ctx->ebp = 0;
	ctx->eflags = 0x202;
	ctx->eip = (uint32_t)thread_trampoline;
	t->context = ctx;
}

struct thread *
thread_create(struct task *task, void (*entry)(void))
{
	struct thread *t = thread_new(task);

	t->entry = entry;
	thread_init_context(t);

	return t;
}

/*
 * entry_va/ustack_va sao endereços de RING3 - quem chama ja precisa
 * ter mapeado essas paginas com PAGE_USER antes (vmm_map(), ver o
 * exemplo em main.c). a stack de kernel (t->stack) e separada dessa
 * e sempre existe: e nela que a thread comeca a rodar (ainda em
 * ring0, dentro do trampolim) antes do user_enter() saltar pra
 * ring3, e e ela que o tss aponta pra quando essa thread voltar pro
 * kernel por interrupcao.
 */
struct thread *
thread_create_user(struct task *task, uint32_t entry_va, uint32_t ustack_va)
{
	struct thread *t = thread_new(task);

	t->user_entry = entry_va;
	t->user_stack = ustack_va;
	thread_init_context(t);

	return t;
}

/*
 * tudo que precisa acontecer quando "current" passa a ser outra
 * thread - hoje so isso, mas e o mesmo lugar onde um cr3 por-task
 * entraria no dia que tiver address space separada por task.
 */
static void
switch_to(struct thread *t)
{
	t->state = THREAD_RUNNING;
	current = t;
	tss_set_kstack((uint32_t)(t->stack + t->stack_size));
}

void
yield(void)
{
	struct thread *prev = current;

	if (prev == NULL || prev->next == prev)
		return;		/* nenhuma ou so uma thread - nada pra trocar */

	prev->state = THREAD_READY;
	switch_to(prev->next);

	swtch(&prev->context, current->context);
}

/*
 * mesma troca do yield(), chamada de dentro do hardclock() (irq0,
 * amd64/pit.c) em vez de por uma thread pedindo de proposito - dai
 * "preemptivo": quem estava rodando nao tem escolha. o swtch() em
 * si nao liga pra quem chamou; a unica diferenca de verdade e o
 * estado da cpu no momento (dentro de uma interrupt gate, IF ja
 * desligado pela cpu) - e exatamente pra isso que existe o
 * pushf/popf de eflags (ver machine/context.h).
 */
void
scheduler_tick(void)
{
	yield();
}

void
scheduler_start(void)
{
	struct context *unused;

	if (run_queue == NULL)
		panic("scheduler_start: nenhuma thread criada");

	switch_to(run_queue);

	/* primeira troca: nao tem thread "anterior" de verdade pra
	   salvar, entao o contexto salvo em unused e descartado */
	swtch(&unused, current->context);

	/* so chega aqui se todas as threads morrerem algum dia -
	   ainda nao existe essa nocao, entao isso e inalcancavel hoje */
	panic("scheduler_start: escalonador voltou (nao deveria)");
}
