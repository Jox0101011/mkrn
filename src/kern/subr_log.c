/*
 * subr_log.c - logging padronizado do kernel
 *
 * klog/kwarn/kerror so diferem no rotulo antes do timestamp; toda a
 * montagem de "[ rotulo | timestamp ] facilidade: " passa por
 * klog_prefix() pra nao repetir a mesma coisa tres vezes.
 */

#include "sys/types.h"
#include <stdarg.h>

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
	klog_prefix(NULL, fac);
	kvprintf(fmt, ap);
	kputc('\n');
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

	klog_prefix("WARN", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
}

void
kerror(const char *fac, const char *fmt, ...)
{
	va_list ap;

	klog_prefix("ERROR", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
}
