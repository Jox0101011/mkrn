/* ls - list directory contents (POSIX.1-2008) */
#include "ls.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *argv0;
int Aflag, aflag, Fflag, Hflag, Lflag, Rflag, Sflag;
int cflag, dflag, fflag, gflag, iflag, kflag, lflag, nflag, oflag;
int pflag, qflag, rflag, sflag, tflag, uflag;
int outfmt = FMT_DEFAULT, timesel = TIME_MTIME, status;
long blocksize = 512;

static int first = 1;		/* nothing written yet: suppress leading blank line */

struct visited { dev_t dev; ino_t ino; };
static struct visited *stack;
static size_t stackn, stackcap;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-ACFHLRSacdfgiklmnopqrstux1] [file...]\n", argv0);
	exit(1);
}

void *
xmalloc(size_t n)
{
	void *p = malloc(n);

	if (p == NULL) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		exit(1);
	}
	return p;
}

void *
xrealloc(void *p, size_t n)
{
	if ((p = realloc(p, n)) == NULL) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		exit(1);
	}
	return p;
}

char *
xstrdup(const char *s)
{
	char *p = xmalloc(strlen(s) + 1);

	strcpy(p, s);
	return p;
}

char *
joinpath(const char *dir, const char *name)
{
	size_t dn = strlen(dir);
	char *p;

	while (dn > 1 && dir[dn - 1] == '/')
		dn--;
	p = xmalloc(dn + 1 + strlen(name) + 1);
	memcpy(p, dir, dn);
	p[dn] = '/';
	strcpy(p + dn + 1, name);
	return p;
}

void
warn2(const char *path, const char *msg)
{
	fprintf(stderr, "%s: %s: %s\n", argv0, path, msg);
	status = 1;
}

/* whether to follow (stat) a symlink at this depth; 0 is a command-line operand */
int
followsym(int depth)
{
	if (Lflag)
		return 1;
	if (depth == 0)
		return Hflag || !(dflag || Fflag || lflag || nflag || gflag || oflag);
	return 0;
}

time_t
entrytime(const struct stat *st)
{
	if (timesel == TIME_CTIME)
		return st->st_ctime;
	if (timesel == TIME_ATIME)
		return st->st_atime;
	return st->st_mtime;
}

static char *
readlinkdup(const char *path)
{
	char buf[PATH_MAX];
	ssize_t n;

	if ((n = readlink(path, buf, sizeof buf - 1)) < 0)
		return NULL;
	buf[n] = '\0';
	return xstrdup(buf);
}

static int
makeentry(struct entry *e, const char *dir, const char *name, int depth)
{
	e->name = xstrdup(name);
	e->path = dir ? joinpath(dir, name) : xstrdup(name);
	if ((followsym(depth) ? stat(e->path, &e->st) : lstat(e->path, &e->st)) < 0) {
		warn2(e->path, strerror(errno));
		free(e->name);
		free(e->path);
		return -1;
	}
	e->link = S_ISLNK(e->st.st_mode) ? readlinkdup(e->path) : NULL;
	return 0;
}

static void
freeentry(struct entry *e)
{
	free(e->name);
	free(e->path);
	free(e->link);
}

static void
header(const char *label)
{
	printf("%s%s:\n", first ? "" : "\n", label);
	first = 0;
}

/* -R: track the (dev,ino) of every directory on the current descent path */
static int
pushvisit(dev_t dev, ino_t ino)
{
	size_t i;

	for (i = 0; i < stackn; i++)
		if (stack[i].dev == dev && stack[i].ino == ino)
			return -1;
	if (stackn == stackcap) {
		stackcap = stackcap ? stackcap * 2 : 8;
		stack = xrealloc(stack, stackcap * sizeof *stack);
	}
	stack[stackn].dev = dev;
	stack[stackn].ino = ino;
	stackn++;
	return 0;
}

static void
popvisit(void)
{
	stackn--;
}

static void
listdir(const char *path, const char *label, int depth, int wantheader)
{
	DIR *dp;
	struct dirent *de;
	struct entry *ents = NULL;
	size_t n = 0, cap = 0, i;

	if ((dp = opendir(path)) == NULL) {
		warn2(path, strerror(errno));
		return;
	}
	if (wantheader)
		header(label);

	while ((de = readdir(dp)) != NULL) {
		int isdotdot = strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0;
		int hidden = de->d_name[0] == '.';

		if (isdotdot) {
			if (!aflag)
				continue;
		} else if (hidden && !(aflag || Aflag)) {
			continue;
		}

		if (n == cap) {
			cap = cap ? cap * 2 : 32;
			ents = xrealloc(ents, cap * sizeof *ents);
		}
		if (makeentry(&ents[n], path, de->d_name, depth + 1) == 0)
			n++;
	}
	closedir(dp);

	sortentries(ents, n, 0);
	printentries(ents, n, 1);

	if (Rflag) {
		for (i = 0; i < n; i++) {
			int isdotdot = strcmp(ents[i].name, ".") == 0 ||
			    strcmp(ents[i].name, "..") == 0;

			if (isdotdot || !S_ISDIR(ents[i].st.st_mode))
				continue;
			if (pushvisit(ents[i].st.st_dev, ents[i].st.st_ino) < 0) {
				warn2(ents[i].path, "not descending into loop");
				continue;
			}
			listdir(ents[i].path, ents[i].path, depth + 1, 1);
			popvisit();
		}
	}

	for (i = 0; i < n; i++)
		freeentry(&ents[i]);
	free(ents);
}

int
main(int argc, char *argv[])
{
	static char *dotv[] = { ".", NULL };
	struct entry *plain = NULL, *dirs = NULL;
	size_t nplain = 0, cplain = 0, ndirs = 0, cdirs = 0, i;
	int ch, topheader;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "AaCcdFfgHiklLmnopqRrSstux1")) != -1) {
		switch (ch) {
		case 'A': Aflag = 1; break;
		case 'a': aflag = 1; break;
		case 'C': outfmt = FMT_COLS; break;
		case 'c': timesel = TIME_CTIME; break;
		case 'd': dflag = 1; break;
		case 'F': Fflag = 1; break;
		case 'f': fflag = 1; break;
		case 'g': gflag = 1; outfmt = FMT_LONG; break;
		case 'H': Hflag = 1; Lflag = 0; break;
		case 'i': iflag = 1; break;
		case 'k': kflag = 1; break;
		case 'l': lflag = 1; outfmt = FMT_LONG; break;
		case 'L': Lflag = 1; Hflag = 0; break;
		case 'm': outfmt = FMT_STREAM; break;
		case 'n': nflag = 1; outfmt = FMT_LONG; break;
		case 'o': oflag = 1; outfmt = FMT_LONG; break;
		case 'p': pflag = 1; break;
		case 'q': qflag = 1; break;
		case 'R': Rflag = 1; break;
		case 'r': rflag = 1; break;
		case 'S': Sflag = 1; tflag = 0; break;
		case 's': sflag = 1; break;
		case 't': tflag = 1; Sflag = 0; break;
		case 'u': timesel = TIME_ATIME; break;
		case 'x': outfmt = FMT_ACROSS; break;
		case '1': outfmt = FMT_ONE; break;
		default: usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (fflag) {
		lflag = nflag = gflag = oflag = 0;
		tflag = Sflag = sflag = rflag = 0;
		aflag = 1;
		if (outfmt == FMT_LONG)
			outfmt = FMT_DEFAULT;
	}
	blocksize = kflag ? 1024 : 512;

	if (argc == 0) {
		argv = dotv;
		argc = 1;
	}

	for (i = 0; i < (size_t)argc; i++) {
		struct entry e;

		if (makeentry(&e, NULL, argv[i], 0) < 0)
			continue;
		if (!dflag && S_ISDIR(e.st.st_mode)) {
			if (ndirs == cdirs) {
				cdirs = cdirs ? cdirs * 2 : 8;
				dirs = xrealloc(dirs, cdirs * sizeof *dirs);
			}
			dirs[ndirs++] = e;
		} else {
			if (nplain == cplain) {
				cplain = cplain ? cplain * 2 : 8;
				plain = xrealloc(plain, cplain * sizeof *plain);
			}
			plain[nplain++] = e;
		}
	}

	if (nplain > 0) {
		sortentries(plain, nplain, 1);
		printentries(plain, nplain, 0);
		first = 0;
	}

	topheader = (ndirs + (nplain ? 1 : 0)) > 1;
	sortentries(dirs, ndirs, 1);
	for (i = 0; i < ndirs; i++) {
		pushvisit(dirs[i].st.st_dev, dirs[i].st.st_ino);
		listdir(dirs[i].path, dirs[i].name, 0, topheader);
		popvisit();
	}

	return status;
}
