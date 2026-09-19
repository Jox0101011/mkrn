/*
 * prf.h - printf minimo do kernel
 */

#ifndef _SYS_PRF_H_
#define _SYS_PRF_H_

#include <stdarg.h>

void kprintf(const char *fmt, ...);
void kvprintf(const char *fmt, va_list ap);

#endif /* !_SYS_PRF_H_ */
