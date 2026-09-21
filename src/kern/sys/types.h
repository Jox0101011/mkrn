/*
 * types.h - tipos basicos do kernel
 *
 * nao usa <stdint.h>/<stddef.h> do host de proposito: em toolchains
 * diferentes (glibc, musl, etc) esses headers podem vir de lugares
 * diferentes ou depender de coisas que nao existem num ambiente
 * freestanding, e isso ja causou erro de build fora daqui. os tipos
 * abaixo nao dependem de nada alem da linguagem C em si.
 *
 * <stdarg.h> continua vindo do compilador (nao tem como reescrever
 * va_list/va_arg de forma portavel, e o gcc garante ele mesmo como
 * freestanding) - o Makefile usa -nostdinc mais -idirafter no
 * diretorio interno do proprio gcc, entao so esse header (e nada
 * mais do sistema) fica alcancavel.
 */

#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

typedef unsigned char		uint8_t;
typedef signed char		int8_t;
typedef unsigned short		uint16_t;
typedef signed short		int16_t;
typedef unsigned int		uint32_t;
typedef signed int		int32_t;
typedef unsigned long long	uint64_t;
typedef signed long long	int64_t;

/* alvo e sempre i386 (ver Makefile: -m32), ponteiro cabe em 32 bits */
typedef uint32_t		uintptr_t;
typedef int32_t		intptr_t;
typedef uint32_t		size_t;
typedef int32_t		ssize_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* !_SYS_TYPES_H_ */
