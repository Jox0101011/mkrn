/* cat - concatenate and print files (POSIX.1-2008) */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *argv0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-u] [file ...]\n", argv0);
	exit(1);
}

/* copy fd to stdout, return 0 on success */
static int
cat(int fd, const char *name)
{
	char buf[BUFSIZ];
	ssize_t n, w, off;

	while ((n = read(fd, buf, sizeof buf)) > 0) {
		for (off = 0; off < n; off += w) {
			w = write(STDOUT_FILENO, buf + off, n - off);
			if (w < 0) {
				fprintf(stderr, "%s: write error: %s\n",
				    argv0, strerror(errno));
				return 1;
			}
		}
	}
	if (n < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, name, strerror(errno));
		return 1;
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int fd, ch, status = 0;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "u")) != -1) {
		switch (ch) {
		case 'u':
			break; /* we never buffer, so -u is a no-op */
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		return cat(STDIN_FILENO, "stdin");

	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-") == 0) {
			status |= cat(STDIN_FILENO, "stdin");
			continue;
		}
		if ((fd = open(*argv, O_RDONLY)) < 0) {
			fprintf(stderr, "%s: %s: %s\n",
			    argv0, *argv, strerror(errno));
			status |= 1;
			continue;
		}
		status |= cat(fd, *argv);
		close(fd);
	}

	return status;
}
