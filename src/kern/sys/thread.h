/*
 * thread.h - task, thread e o escalonador
 *
 *   task
 *    ├── address space   (pgdir_phys - por enquanto so a do kernel)
 *    ├── capability table (captbl - ainda nao existe, NULL)
 *    ├── ipc space        (ipc - ainda nao existe, NULL)
 *    └── threads
 *          ├── thread A
 *          ├── thread B
 *          └── ...
 *
 * cada thread tem eip/esp/registradores (tudo dentro de struct
 * context, salvo/restaurado por amd64/switch.S - so declarado por
 * ponteiro aqui, quem precisa do layout de verdade inclui
 * machine/context.h), uma stack propria, e um estado de
 * escalonamento.
 *
 * o escalonador e cooperativo, round-robin, numa fila circular:
 * yield() e a unica forma de trocar de thread agora. preempcao de
 * verdade (o timer interrompendo quem nao chamou yield) fica pra
 * depois - amd64/pit.c ja tem o gancho comentado esperando por
 * isso.
 */

#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include "types.h"


struct context;		/* amd64/include/machine/context.h */

enum thread_state {
	THREAD_READY,
	THREAD_RUNNING,
	THREAD_BLOCKED,		/* nao usado ainda - nada bloqueia hoje */
	THREAD_DEAD,
};

struct thread {
	struct context		*context;
	uint8_t			*stack;
	size_t			stack_size;
	void			(*entry)(void);
	enum thread_state	state;
	struct task		*task;		/* task dona desta thread */
	struct thread		*next;		/* fila circular de escalonamento */
	struct thread		*task_next;	/* proxima na lista de threads da task */
};

struct task {
	uint32_t		pgdir_phys;	/* cr3 do address space desta task */
	void			*captbl;	/* tabela de capabilities - reservado */
	void			*ipc;		/* ipc space - reservado */
	struct thread		*threads;
	struct task		*next;
};

extern struct task kernel_task;

void sched_init(void);
void scheduler_start(void);		/* nunca retorna */

struct thread *thread_create(struct task *task, void (*entry)(void));
void yield(void);

#endif /* !_SYS_THREAD_H_ */
