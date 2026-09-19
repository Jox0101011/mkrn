/*
 * panic.h - parada de emergencia do kernel
 */

#ifndef _SYS_PANIC_H_
#define _SYS_PANIC_H_

void panic(const char *fmt, ...) __attribute__((noreturn));

#endif /* !_SYS_PANIC_H_ */
