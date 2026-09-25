/*
 * subr_log.c - logging padronizado do kernel
 *
 * klog/kwarn/kerror so diferem no rotulo antes do timestamp; toda a
 * montagem de "[ rotulo | timestamp ] facilidade: " passa por
 * klog_prefix() pra nao repetir a mesma coisa tres vezes.
 *
 * cada linha vai inteira entre cons_log(1)/cons_log(0): e o que
 * pinta so as celulas de log (nao a tela toda) de preto sobre
 * branco, pra diferenciar visualmente do resto (ver sys/cons.h).
 *
 * fb_row/fb_col (e vga_row/vga_col) sao estado global, sem
 * trava nenhuma - com o escalonador preemptivo (subr_thread.c),
 * o timer pode cortar uma thread no meio de uma mensagem e trocar
 * pra outra que tambem chama klog(): as duas escrevem na mesma
 * posicao de cursor ao mesmo tempo, linhas se misturando (e ate um
 * fb_scroll() cortado no meio por outro em cima, dando aquelas
 * listras). por isso cli_save()/sti_restore() em volta de cada
 * linha inteira - do jeito que kmalloc() ja fazia com o heap antes
 * de ter preempcao de thread, so que agora e por essa razao.
 */

#include "sys/types.h"
#include <stdarg.h>

#include "amd64/include/machine/cpufunc.h"
#include "sys/clock.h"
#include "sys/cons.h"
#include "sys/log.h"
#include "sys/prf.h"

static void
klog_prefix(const char *level, const char *fac)
{
	unsigned long sec, usec;

	md_uptime(&sec, &usec);

	if (level != NULL)
		kprintf("[%s | %5lu.%06lu] ", level, sec, usec);
	else
		kprintf("[%5lu.%06lu] ", sec, usec);

	if (fac != NULL)
		kprintf("%s: ", fac);
}

void
vklog(const char *fac, const char *fmt, va_list ap)
{
	uint32_t flags = cli_save();

	cons_log(1);
	klog_prefix(NULL, fac);
	kvprintf(fmt, ap);
	kputc('\n');
	cons_log(0);

	sti_restore(flags);
}

void
klog(const char *fac, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vklog(fac, fmt, ap);
	va_end(ap);
}

void
kwarn(const char *fac, const char *fmt, ...)
{
	va_list ap;
	uint32_t flags = cli_save();

	cons_log(1);
	klog_prefix("WARN", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
	cons_log(0);

	sti_restore(flags);
}

void
kerror(const char *fac, const char *fmt, ...)
{
	va_list ap;
	uint32_t flags = cli_save();

	cons_log(1);
	klog_prefix("ERROR", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
	cons_log(0);

	sti_restore(flags);
}
