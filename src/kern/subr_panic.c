/*
 * subr_panic.c - erro fatal do kernel
 *
 * mesmo esquema de logging do klog()/vklog(), so que com interrupcoes
 * desligadas e sem volta: depois de logar, trava a cpu de vez.
 */

#include <stdarg.h>

#include "sys/log.h"
#include "sys/panic.h"

void
panic(const char *fmt, ...)
{
	static int panicking = 0;
	va_list ap;

	__asm__ volatile("cli");

	if (panicking) {
		/* panic dentro de panic - nao arrisca chamar vklog() de
		   novo (pode ser exatamente o que esta quebrado), so trava */
		for (;;)
			__asm__ volatile("hlt");
	}
	panicking = 1;

	va_start(ap, fmt);
	vklog("panic", fmt, ap);
	va_end(ap);

	for (;;)
		__asm__ volatile("hlt");
}
