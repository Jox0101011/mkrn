/* ls.h - shared declarations for ls.c and format.c */
#ifndef LS_H
#define LS_H

#include <sys/stat.h>
#include <sys/types.h>

/* one file to be listed */
struct entry {
	char *name;	/* name as shown to the user */
	char *path;	/* path usable for stat()/readlink() */
	struct stat st;	/* lstat(), or stat() where the link rules say to follow */
	char *link;	/* readlink() target, or NULL if not a kept symlink */
};

enum { FMT_DEFAULT, FMT_COLS, FMT_ACROSS, FMT_STREAM, FMT_LONG, FMT_ONE };
enum { TIME_MTIME, TIME_CTIME, TIME_ATIME };
enum { SORT_NAME, SORT_TIME, SORT_SIZE };

extern char *argv0;
extern int Aflag, aflag, Fflag, Hflag, Lflag, Rflag, Sflag;
extern int cflag, dflag, fflag, gflag, iflag, kflag, lflag, nflag, oflag;
extern int pflag, qflag, rflag, sflag, tflag, uflag;
extern int outfmt, timesel, status;
extern long blocksize;

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *joinpath(const char *dir, const char *name);
void warn2(const char *path, const char *msg);

int followsym(int depth);
time_t entrytime(const struct stat *st);

/* format.c */
void sortentries(struct entry *ents, size_t n, int nameonly);
void printentries(struct entry *ents, size_t n, int withtotal);

#endif
