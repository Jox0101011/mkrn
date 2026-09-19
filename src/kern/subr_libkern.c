/*
 * subr_libkern.c - implementacoes basicas de mem
 * */

/*
 * str* pro kernel
 *
 * sem otimizacoes (sem sse, sem copia word-a-word) - byte a byte
 * mesmo, simples e correto. da pra trocar por versoes mais rapidas
 * depois, se precisar (o compilador nao vai substituir sozinho
 * por builtin por causa do -fno-builtin).
 */

#include <stddef.h>

#include "sys/libkern.h"

void *
memcpy(void *dst, const void *src, size_t len)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	while (len-- > 0)
		*d++ = *s++;

	return dst;
}

void *
memmove(void *dst, const void *src, size_t len)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	if (d == s || len == 0)
		return dst;

	if (d < s) {
		while (len-- > 0)
			*d++ = *s++;
	} else {
		d += len;
		s += len;
		while (len-- > 0)
			*--d = *--s;
	}

	return dst;
}

void *
memset(void *dst, int c, size_t len)
{
	unsigned char *d = dst;

	while (len-- > 0)
		*d++ = (unsigned char)c;

	return dst;
}

int
memcmp(const void *a, const void *b, size_t len)
{
	const unsigned char *pa = a, *pb = b;

	while (len-- > 0) {
		if (*pa != *pb)
			return (int)*pa - (int)*pb;
		pa++;
		pb++;
	}

	return 0;
}

size_t
strlen(const char *s)
{
	size_t n = 0;

	while (s[n] != '\0')
		n++;

	return n;
}

char *
strcpy(char *dst, const char *src)
{
	char *ret = dst;

	while ((*dst++ = *src++) != '\0')
		continue;

	return ret;
}

char *
strncpy(char *dst, const char *src, size_t len)
{
	size_t i;

	for (i = 0; i < len && src[i] != '\0'; i++)
		dst[i] = src[i];
	for (; i < len; i++)
		dst[i] = '\0';

	return dst;
}

int
strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}

	return (unsigned char)*a - (unsigned char)*b;
}

int
strncmp(const char *a, const char *b, size_t len)
{
	while (len-- > 0) {
		if (*a != *b || *a == '\0')
			return (unsigned char)*a - (unsigned char)*b;
		a++;
		b++;
	}

	return 0;
}

char *
strcat(char *dst, const char *src)
{
	char *ret = dst;

	while (*dst != '\0')
		dst++;

	while ((*dst++ = *src++) != '\0')
		continue;

	return ret;
}

char *
strchr(const char *s, int c)
{
	while (*s != '\0') {
		if (*s == (char)c)
			return (char *)s;
		s++;
	}

	return (c == '\0') ? (char *)s : NULL;
}
