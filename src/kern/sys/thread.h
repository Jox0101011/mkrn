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
 * o escalonador e round-robin numa fila circular, trocado por
 * yield() (cooperativo, quem chama decide a hora) ou por
 * scheduler_tick() (preemptivo, chamado pelo hardclock() do timer
 * em amd64/pit.c - cada irq0 e uma fatia de tempo). os dois caem no
 * mesmo swtch(); pra quem esta sendo trocado nao tem diferenca
 * nenhuma entre os dois motivos.
 *
 * thread de kernel (thread_create) x thread de usuario
 * (thread_create_user): as duas tem a MESMA stack de kernel (e ela
 * que tss_set_kstack() aponta em cada troca - ver amd64/gdt.c) e
 * passam pelo mesmo swtch()/trampolim. a diferenca e so o que o
 * trampolim faz na primeira vez que a thread roda: uma thread de
 * kernel chama entry() direto, ainda em ring0; uma de usuario chama
 * user_enter() (amd64/user_enter.S), que monta o frame de iret com
 * user_entry/user_stack e salta pra ring3 - dali em diante essa
 * thread so volta pro kernel por interrupcao (excecao ou o timer),
 * nunca por retorno de chamada.
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
	uint8_t			*stack;		/* stack de KERNEL (ring0) - toda thread tem, mesmo as de usuario */
	size_t			stack_size;
	void			(*entry)(void);	/* thread de kernel: chamada direto em ring0 */
	uint32_t		user_entry;	/* !=0: thread de usuario - eip de entrada em ring3 */
	uint32_t		user_stack;	/* esp de entrada em ring3 (topo da stack de usuario) */
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

struct task *task_create(void);
struct thread *thread_create(struct task *task, void (*entry)(void));
struct thread *thread_create_user(struct task *task, uint32_t entry_va, uint32_t ustack_va);
void yield(void);
void scheduler_tick(void);	/* chamado pelo hardclock() a cada irq0 - amd64/pit.c */

#endif /* !_SYS_THREAD_H_ */
