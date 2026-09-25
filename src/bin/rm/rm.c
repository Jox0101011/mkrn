/* rm - remove directory entries (POSIX.1-2008) */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *argv0;
static int fflag, iflag, Rflag, status;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-fiRr] file...\n", argv0);
	exit(1);
}

/* prompt on stderr, read a line from stdin, true if it starts with y/Y */
static int
confirm(const char *verb, const char *path)
{
	int c, yes;

	fprintf(stderr, "%s: %s '%s'? ", argv0, verb, path);
	yes = (c = getchar()) == 'y' || c == 'Y';
	while (c != '\n' && c != EOF)
		c = getchar();
	return yes;
}

/* true if the last pathname component is "." or ".." */
static int
isdotname(const char *path)
{
	const char *b = strrchr(path, '/');

	b = b ? b + 1 : path;
	return strcmp(b, ".") == 0 || strcmp(b, "..") == 0;
}

/* true if path, symlinks resolved, is the root directory */
static int
isroot(const char *path)
{
	char resolved[PATH_MAX];

	return realpath(path, resolved) != NULL && strcmp(resolved, "/") == 0;
}

static void rm(const char *path);

/* remove every entry of a directory, except . and .. */
static void
rmtree(const char *path)
{
	DIR *dp;
	struct dirent *de;
	char child[PATH_MAX];

	if ((dp = opendir(path)) == NULL) {
		fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
		status = 1;
		return;
	}
	while ((de = readdir(dp)) != NULL) {
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
		if ((size_t)snprintf(child, sizeof child, "%s/%s",
		    path, de->d_name) >= sizeof child) {
			fprintf(stderr, "%s: %s/%s: pathname too long\n",
			    argv0, path, de->d_name);
			status = 1;
			continue;
		}
		rm(child);
	}
	closedir(dp);
}

/* remove one file, or with -R/-r a whole hierarchy */
static void
rm(const char *path)
{
	struct stat st;
	int ask;

	/* refuse dot, dotdot, and anything that resolves to the root */
	if (isdotname(path) || isroot(path)) {
		fprintf(stderr, "%s: refusing to remove '%s'\n", argv0, path);
		status = 1;
		return;
	}

	if (lstat(path, &st) < 0) {
		if (!fflag) {
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			status = 1;
		}
		return;
	}

	ask = !fflag && ((access(path, W_OK) < 0 && isatty(STDIN_FILENO)) || iflag);

	if (S_ISDIR(st.st_mode)) {
		if (!Rflag) {
			fprintf(stderr, "%s: %s: is a directory\n", argv0, path);
			status = 1;
			return;
		}
		if (ask && !confirm("descend into directory", path))
			return;

		rmtree(path);

		if (iflag && !confirm("remove directory", path))
			return;
		if (rmdir(path) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			status = 1;
		}
	} else {
		if (ask && !confirm("remove", path))
			return;
		if (unlink(path) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, path, strerror(errno));
			status = 1;
		}
	}
}

int
main(int argc, char *argv[])
{
	int ch;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "fiRr")) != -1) {
		switch (ch) {
		case 'f':
			fflag = 1;
			iflag = 0;
			break;
		case 'i':
			iflag = 1;
			fflag = 0;
			break;
		case 'R':
		case 'r':
			Rflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	for (; argc > 0; argc--, argv++)
		rm(*argv);

	return status;
}
