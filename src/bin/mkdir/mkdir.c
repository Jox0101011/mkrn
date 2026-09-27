/* mkdir - make directories (POSIX.1-2008) */
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *argv0;
static int status;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-p] [-m mode] dir...\n", argv0);
	exit(1);
}

static char *
xstrdup(const char *s)
{
	char *p = strdup(s);

	if (p == NULL) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		exit(1);
	}
	return p;
}

/* one clause of a symbolic mode, e.g. "u+wx" or "=rwx"; applied to *m */
static int
applyclause(mode_t *m, const char *clause)
{
	const char *p = clause;
	int who = 0, op;
	mode_t bits;

	while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
		who |= (*p == 'u') ? 1 : (*p == 'g') ? 2 : (*p == 'o') ? 4 : 7;
		p++;
	}
	if (who == 0)
		who = 7;	/* mkdir -m has no umask exception: omitted who means "a" */

	for (;;) {
		if (*p != '+' && *p != '-' && *p != '=')
			return -1;
		op = *p++;

		bits = 0;
		for (; *p != '\0' && strchr("rwxXst", *p) != NULL; p++) {
			switch (*p) {
			case 'r':
				if (who & 1) bits |= S_IRUSR;
				if (who & 2) bits |= S_IRGRP;
				if (who & 4) bits |= S_IROTH;
				break;
			case 'w':
				if (who & 1) bits |= S_IWUSR;
				if (who & 2) bits |= S_IWGRP;
				if (who & 4) bits |= S_IWOTH;
				break;
			case 'x':
			case 'X':	/* mkdir's target is always a directory: X == x */
				if (who & 1) bits |= S_IXUSR;
				if (who & 2) bits |= S_IXGRP;
				if (who & 4) bits |= S_IXOTH;
				break;
			case 's':
				if (who & 1) bits |= S_ISUID;
				if (who & 2) bits |= S_ISGID;
				break;
			case 't':
				bits |= S_ISVTX;
				break;
			}
		}

		if (op == '+') {
			*m |= bits;
		} else if (op == '-') {
			*m &= ~bits;
		} else {
			if (who & 1) *m &= ~(S_IRWXU | S_ISUID);
			if (who & 2) *m &= ~(S_IRWXG | S_ISGID);
			if (who & 4) *m &= ~(S_IRWXO | S_ISVTX);
			*m |= bits;
		}

		if (*p != '+' && *p != '-' && *p != '=')
			break;
	}
	return *p == '\0' ? 0 : -1;
}

/* mkdir -m always starts from a=rwx, per its own override of chmod's rule */
static int
parsemode(const char *s, mode_t *out)
{
	char *end, *dup, *tok, *save;
	long v;

	if (isdigit((unsigned char)s[0])) {
		v = strtol(s, &end, 8);
		if (*end != '\0' || v < 0 || v > 07777)
			return -1;
		*out = (mode_t)v;
		return 0;
	}

	*out = S_IRWXU | S_IRWXG | S_IRWXO;
	dup = xstrdup(s);
	for (tok = strtok_r(dup, ",", &save); tok != NULL; tok = strtok_r(NULL, ",", &save)) {
		if (applyclause(out, tok) < 0) {
			free(dup);
			return -1;
		}
	}
	free(dup);
	return 0;
}

/* create a single path component. Without -p an existing directory is a
 * real error (plain mkdir(2) semantics); with -p it is left untouched. */
static int
mkone(const char *path, int isfinal, int ignoreexist, int has_m, mode_t modearg)
{
	struct stat st;

	if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
		if (ignoreexist && errno == EEXIST && stat(path, &st) == 0) {
			if (S_ISDIR(st.st_mode))
				return 0;	/* -p: already there, leave it alone */
			fprintf(stderr, "%s: %s: not a directory\n", argv0, path);
			return -1;
		}
		fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
		return -1;
	}

	if (stat(path, &st) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
		return -1;
	}

	if (isfinal) {
		/* only touch it if -m asked for bits umask didn't already leave us with */
		if (has_m && (st.st_mode & 07777) != modearg && chmod(path, modearg) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			return -1;
		}
	} else {
		/* intermediate component: guarantee it stays usable regardless of umask */
		if ((st.st_mode & (S_IWUSR | S_IXUSR)) != (S_IWUSR | S_IXUSR) &&
		    chmod(path, (st.st_mode | S_IWUSR | S_IXUSR) & 07777) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			return -1;
		}
	}
	return 0;
}

/* -p: walk dir component by component, creating whatever is missing */
static int
makepath(const char *dir, int has_m, mode_t modearg)
{
	char *dup = xstrdup(dir);
	char *p, *slash;
	size_t len = strlen(dup);
	int rv = 0;

	while (len > 1 && dup[len - 1] == '/')
		dup[--len] = '\0';

	p = dup;
	while (*p == '/')
		p++;

	for (;;) {
		slash = strchr(p, '/');
		if (slash != NULL)
			*slash = '\0';

		if (dup[0] != '\0' && strcmp(dup, ".") != 0 &&
		    mkone(dup, slash == NULL, 1, has_m, modearg) < 0) {
			rv = -1;
			break;
		}

		if (slash == NULL)
			break;
		*slash = '/';
		p = slash + 1;
		while (*p == '/')
			p++;
	}
	free(dup);
	return rv;
}

int
main(int argc, char *argv[])
{
	int ch, i, pflag = 0, has_m = 0;
	mode_t modearg = 0;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "pm:")) != -1) {
		switch (ch) {
		case 'p': pflag = 1; break;
		case 'm':
			if (parsemode(optarg, &modearg) < 0) {
				fprintf(stderr, "%s: invalid mode '%s'\n", argv0, optarg);
				exit(1);
			}
			has_m = 1;
			break;
		default: usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0)
		usage();

	for (i = 0; i < argc; i++) {
		if (pflag) {
			if (makepath(argv[i], has_m, modearg) < 0)
				status = 1;
		} else if (mkone(argv[i], 1, 0, has_m, modearg) < 0) {
			status = 1;
		}
	}

	return status;
}
