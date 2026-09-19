/*
 * subr_log.c - logging padronizado do kernel
 */

#include <stdarg.h>
#include <stddef.h>

#include "sys/clock.h"
#include "sys/cons.h"
#include "sys/log.h"
#include "sys/prf.h"

void
klog(const char *fac, const char *fmt, ...)
{
	unsigned long sec, usec;
	va_list ap;

	md_uptime(&sec, &usec);
	kprintf("[%5lu.%06lu] ", sec, usec);

	if (fac != NULL)
		kprintf("%s: ", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
}

void
kwarn(const char *fac, const char *fmt, ...)
{
	unsigned long sec, usec;
	va_list ap;

	md_uptime(&sec, &usec);
	kprintf("[WARN | %5lu.%06lu] ", sec, usec);

	if (fac != NULL)
		kprintf("%s: ", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
}

void
kerror(const char *fac, const char *fmt, ...)
{	
    unsigned long sec, usec;
	va_list ap;

	md_uptime(&sec, &usec);
	kprintf("[ERROR | %5lu.%06lu] ", sec, usec);

	if (fac != NULL)
		kprintf("%s: ", fac);

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);

	kputc('\n');
} 
