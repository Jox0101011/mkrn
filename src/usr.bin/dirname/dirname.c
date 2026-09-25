/* dirname - return the directory portion of a pathname (POSIX.1-2008) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *argv0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s string\n", argv0);
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *s;
	size_t n, lead;

	argv0 = argv[0];
	if (argc != 2)
		usage();
	s = argv[1];
	n = strlen(s);

	/* step 1: "//" skips steps 2-5 and falls through to 6-8 */
	if (strcmp(s, "//") != 0) {
		/* step 2: string is all slashes -> "/" */
		lead = strspn(s, "/");
		if (lead > 0 && s[lead] == '\0') {
			puts("/");
			return 0;
		}

		/* step 3: drop trailing slashes */
		while (n > 0 && s[n - 1] == '/')
			s[--n] = '\0';

		/* step 4: no slash left -> "." */
		if (strchr(s, '/') == NULL) {
			puts(".");
			return 0;
		}

		/* step 5: drop the trailing (last) component */
		while (n > 0 && s[n - 1] != '/')
			s[--n] = '\0';
	}

	/* steps 6-8: drop trailing slashes, "" becomes "/" */
	while (n > 0 && s[n - 1] == '/')
		s[--n] = '\0';
	puts(n == 0 ? "/" : s);
	return 0;
}
