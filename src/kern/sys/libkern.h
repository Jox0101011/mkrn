/*
 * libkern.h - rotinas basicas de string/memoria do kernel
 *
 * com -fno-builtin e sem libc, o compilador nao tem memcpy/memset/etc
 * pra usar (nem pra otimizacoes proprias, tipo copia de struct) -
 * essas implementacoes cobrem isso. sem SIMD, byte a byte mesmo.
 */

#ifndef _SYS_LIBKERN_H_
#define _SYS_LIBKERN_H_

#include <stddef.h>

void	*memcpy(void *dst, const void *src, size_t len);
void	*memmove(void *dst, const void *src, size_t len);
void	*memset(void *dst, int c, size_t len);
int	 memcmp(const void *a, const void *b, size_t len);

size_t	 strlen(const char *s);
char	*strcpy(char *dst, const char *src);
char	*strncpy(char *dst, const char *src, size_t len);
int	 strcmp(const char *a, const char *b);
int	 strncmp(const char *a, const char *b, size_t len);
char	*strcat(char *dst, const char *src);
char	*strchr(const char *s, int c);

#endif /* !_SYS_LIBKERN_H_ */
