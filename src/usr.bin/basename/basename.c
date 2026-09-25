/* basename - print filename portion of a pathname (POSIX.1-2008) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *argv0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s string [suffix]\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *s, *suffix, *p;
	size_t slen, suflen;

	argv0 = argv[0];
	if (argc < 2 || argc > 3)
		usage();

	s = argv[1];
	suffix = argc == 3 ? argv[2] : NULL;

	/* empty string stays empty */
	if (*s == '\0') {
		putchar('\n');
		return 0;
	}

	/* strip trailing slashes, unless string is all slashes */
	slen = strlen(s);
	while (slen > 1 && s[slen - 1] == '/')
		s[--slen] = '\0';

	/* string was all slashes: result is "/" */
	if (slen == 1 && s[0] == '/') {
		puts("/");
		return 0;
	}

	/* keep only the part after the last slash */
	if ((p = strrchr(s, '/')) != NULL)
		s = p + 1;
	slen = strlen(s);

	/* strip suffix, unless it equals the whole result */
	if (suffix != NULL) {
		suflen = strlen(suffix);
		if (suflen > 0 && suflen < slen &&
		    strcmp(s + slen - suflen, suffix) == 0)
			s[slen - suflen] = '\0';
	}

	puts(s);
	return 0;
}
