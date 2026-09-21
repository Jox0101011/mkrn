/*
 * subr_prf.c - printf minimo do kernel
 *
 * subset de printf(3): %d %i %u %x %p %s %c %%, com o modificador
 * "l" e largura de campo (com ou sem zero-padding). o suficiente
 * pro klog(), nao tenta ser completo.
 */

#include "sys/types.h"
#include <stdarg.h>

#include "sys/cons.h"
#include "sys/prf.h"

#define KPUTNUM_BUFSZ	22	/* cabe um unsigned long em base 2 */
#define WIDTH_MAX	(KPUTNUM_BUFSZ - 2)	/* deixa folga pros digitos de verdade */

static void
kputs(const char *s)
{
	if (s == NULL)
		s = "(null)";

	while (*s != '\0')
		kputc(*s++);
}

static void
kputnum(unsigned long num, unsigned base, int width, int zero)
{
	char buf[KPUTNUM_BUFSZ];
	static const char digits[] = "0123456789abcdef";
	int i = 0;

	if (width > WIDTH_MAX)
		width = WIDTH_MAX;

	do {
		buf[i++] = digits[num % base];
		num /= base;
	} while (num != 0);

	while (i < width)
		buf[i++] = zero ? '0' : ' ';

	while (i > 0)
		kputc(buf[--i]);
}

void
kvprintf(const char *fmt, va_list ap)
{
	int longflag, width, zero;
	long n;

	for (; *fmt != '\0'; fmt++) {
		if (*fmt != '%') {
			kputc(*fmt);
			continue;
		}

		fmt++;
		if (*fmt == '\0')
			break;		/* '%' no fim da string: nada pra formatar */

		zero = (*fmt == '0');
		if (zero)
			fmt++;

		width = 0;
		while (*fmt >= '0' && *fmt <= '9') {
			if (width < 1000)	/* evita overflow de int com formato absurdo */
				width = width * 10 + (*fmt - '0');
			fmt++;
		}

		longflag = (*fmt == 'l');
		if (longflag)
			fmt++;

		if (*fmt == '\0')
			break;		/* formato truncado (ex.: "%5"): nada pra formatar */

		switch (*fmt) {
		case 'd':
		case 'i':
			n = longflag ? va_arg(ap, long) : va_arg(ap, int);
			if (n < 0) {
				kputc('-');
				n = -n;
			}
			kputnum((unsigned long)n, 10, width, zero);
			break;
		case 'u':
			kputnum(longflag ? va_arg(ap, unsigned long) :
			    va_arg(ap, unsigned int), 10, width, zero);
			break;
		case 'x':
			kputnum(longflag ? va_arg(ap, unsigned long) :
			    va_arg(ap, unsigned int), 16, width, zero);
			break;
		case 'p':
			kputs("0x");
			kputnum((uintptr_t)va_arg(ap, void *), 16, 0, 0);
			break;
		case 's':
			kputs(va_arg(ap, char *));
			break;
		case 'c':
			kputc(va_arg(ap, int));
			break;
		case '%':
			kputc('%');
			break;
		default:
			kputc('%');
			kputc(*fmt);
			break;
		}
	}
}

void
kprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
}
