/* format.c - sorting and output formatting for ls */
#include "ls.h"
#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ---- sorting ---- */

static int cur_nameonly;

static int
cmpname(const struct entry *a, const struct entry *b)
{
	return strcoll(a->name, b->name);
}

static int
entrycmp(const void *pa, const void *pb)
{
	const struct entry *a = pa, *b = pb;
	int c;

	if (cur_nameonly || (!Sflag && !tflag)) {
		c = cmpname(a, b);
	} else if (Sflag) {
		if (a->st.st_size != b->st.st_size)
			c = a->st.st_size > b->st.st_size ? -1 : 1;	/* largest first */
		else
			c = cmpname(a, b);
	} else {
		time_t ta = entrytime(&a->st), tb = entrytime(&b->st);

		if (ta != tb)
			c = ta > tb ? -1 : 1;	/* most recent first */
		else
			c = cmpname(a, b);
	}
	return rflag ? -c : c;
}

void
sortentries(struct entry *ents, size_t n, int nameonly)
{
	if (n < 2 || (fflag && !nameonly))
		return;
	cur_nameonly = nameonly;
	qsort(ents, n, sizeof *ents, entrycmp);
}

/* ---- small helpers ---- */

static int
termwidth(void)
{
	char *c = getenv("COLUMNS");
	int w;

	if (c != NULL && (w = atoi(c)) > 0)
		return w;
	return 80;
}

static unsigned long
blocks(const struct entry *e)
{
	/* st_blocks is XSI, counted in 512-byte units */
	return ((unsigned long)e->st.st_blocks * 512 + blocksize - 1) / blocksize;
}

static char
typechar(mode_t m)
{
	if (S_ISDIR(m))
		return 'd';
	if (S_ISLNK(m))
		return 'l';
	if (S_ISBLK(m))
		return 'b';
	if (S_ISCHR(m))
		return 'c';
	if (S_ISFIFO(m))
		return 'p';
#ifdef S_ISSOCK
	if (S_ISSOCK(m))
		return 's';
#endif
	return '-';
}

/* fills 9 permission characters plus nul */
static void
permstr(mode_t m, int isdir, char *buf)
{
	buf[0] = (m & S_IRUSR) ? 'r' : '-';
	buf[1] = (m & S_IWUSR) ? 'w' : '-';
	buf[2] = (m & S_ISUID) ? ((m & S_IXUSR) ? 's' : 'S') : ((m & S_IXUSR) ? 'x' : '-');

	buf[3] = (m & S_IRGRP) ? 'r' : '-';
	buf[4] = (m & S_IWGRP) ? 'w' : '-';
	buf[5] = (m & S_ISGID) ? ((m & S_IXGRP) ? 's' : 'S') : ((m & S_IXGRP) ? 'x' : '-');

	buf[6] = (m & S_IROTH) ? 'r' : '-';
	buf[7] = (m & S_IWOTH) ? 'w' : '-';
#ifdef S_ISVTX
	/* XSI: restricted-deletion (sticky) flag, meaningful for directories only */
	if (isdir && (m & S_ISVTX))
		buf[8] = (m & S_IXOTH) ? 't' : 'T';
	else
#else
	(void)isdir;
#endif
		buf[8] = (m & S_IXOTH) ? 'x' : '-';
	buf[9] = '\0';
}

static char *
ownername(uid_t uid, int numeric)
{
	struct passwd *pw;
	static char buf[32];

	if (!numeric && (pw = getpwuid(uid)) != NULL)
		return pw->pw_name;
	snprintf(buf, sizeof buf, "%u", (unsigned)uid);
	return buf;
}

static char *
groupname(gid_t gid, int numeric)
{
	struct group *gr;
	static char buf[32];

	if (!numeric && (gr = getgrgid(gid)) != NULL)
		return gr->gr_name;
	snprintf(buf, sizeof buf, "%u", (unsigned)gid);
	return buf;
}

static char *
datestr(time_t t)
{
	static char buf[32];
	struct tm tmv;
	double diff = difftime(time(NULL), t);

	localtime_r(&t, &tmv);
	if (diff >= 0 && diff < 15552000)	/* ~6 months */
		strftime(buf, sizeof buf, "%b %e %H:%M", &tmv);
	else
		strftime(buf, sizeof buf, "%b %e  %Y", &tmv);
	return buf;
}

/* -q and -F/-p; caller frees the result */
static char *
dispname(const struct entry *e)
{
	const char *s;
	char *buf, *p;
	char suffix = 0;

	if (Fflag) {
		if (S_ISDIR(e->st.st_mode))
			suffix = '/';
		else if (S_ISLNK(e->st.st_mode))
			suffix = '@';
		else if (S_ISFIFO(e->st.st_mode))
			suffix = '|';
		else if (e->st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
			suffix = '*';
	} else if (pflag && S_ISDIR(e->st.st_mode)) {
		suffix = '/';
	}

	buf = xmalloc(strlen(e->name) + 2);
	p = buf;
	for (s = e->name; *s != '\0'; s++) {
		unsigned char c = (unsigned char)*s;

		*p++ = (qflag && (c == '\t' || !isprint(c))) ? '?' : (char)c;
	}
	if (suffix)
		*p++ = suffix;
	*p = '\0';
	return buf;
}

/* name plus any -i/-s prefix, for the non-long formats; caller frees */
static char *
cellname(const struct entry *e)
{
	char *name = dispname(e);
	char prefix[64];
	char *cell;
	size_t plen = 0;

	prefix[0] = '\0';
	if (iflag)
		plen += snprintf(prefix + plen, sizeof prefix - plen,
		    "%lu ", (unsigned long)e->st.st_ino);
	if (sflag)
		plen += snprintf(prefix + plen, sizeof prefix - plen,
		    "%lu ", blocks(e));

	cell = xmalloc(plen + strlen(name) + 1);
	strcpy(cell, prefix);
	strcat(cell, name);
	free(name);
	return cell;
}

/* ---- the formats themselves ---- */

static void
printtotal(struct entry *ents, size_t n)
{
	unsigned long total512 = 0;
	size_t i;

	for (i = 0; i < n; i++)
		total512 += (unsigned long)ents[i].st.st_blocks;
	printf("total %lu\n", (total512 * 512 + blocksize - 1) / blocksize);
}

static void
printlong(struct entry *ents, size_t n)
{
	struct entry *e;
	char mode[11];
	char *name;
	size_t i;

	for (i = 0; i < n; i++) {
		e = &ents[i];
		mode[0] = typechar(e->st.st_mode);
		permstr(e->st.st_mode, S_ISDIR(e->st.st_mode), mode + 1);

		if (iflag)
			printf("%lu ", (unsigned long)e->st.st_ino);
		if (sflag)
			printf("%lu ", blocks(e));

		printf("%s %u", mode, (unsigned)e->st.st_nlink);
		if (!gflag)
			printf(" %s", ownername(e->st.st_uid, nflag));
		if (!oflag)
			printf(" %s", groupname(e->st.st_gid, nflag));
		printf(" %ju %s ", (uintmax_t)e->st.st_size, datestr(entrytime(&e->st)));

		name = dispname(e);
		if (e->link != NULL)
			printf("%s -> %s\n", name, e->link);
		else
			printf("%s\n", name);
		free(name);
	}
}

static void
printone(struct entry *ents, size_t n)
{
	char *c;
	size_t i;

	for (i = 0; i < n; i++) {
		c = cellname(&ents[i]);
		printf("%s\n", c);
		free(c);
	}
}

static void
printstream(struct entry *ents, size_t n)
{
	int col = 0, w;
	char *c;
	size_t i;

	for (i = 0; i < n; i++) {
		c = cellname(&ents[i]);
		w = (int)strlen(c) + (i + 1 < n ? 2 : 0);
		if (col > 0 && col + w > termwidth()) {
			printf("\n");
			col = 0;
		}
		printf("%s", c);
		col += (int)strlen(c);
		if (i + 1 < n) {
			printf(", ");
			col += 2;
		}
		free(c);
	}
	printf("\n");
}

static void
printcols(struct entry *ents, size_t n, int across)
{
	char **cells = xmalloc(n * sizeof *cells);
	size_t i, maxw = 0, len, idx;
	int ncols, nrows, row, col;

	for (i = 0; i < n; i++) {
		cells[i] = cellname(&ents[i]);
		len = strlen(cells[i]);
		if (len > maxw)
			maxw = len;
	}

	ncols = termwidth() / (int)(maxw + 2);
	if (ncols < 1)
		ncols = 1;
	nrows = (int)((n + (size_t)ncols - 1) / (size_t)ncols);

	for (row = 0; row < nrows; row++) {
		for (col = 0; col < ncols; col++) {
			idx = across ? (size_t)row * ncols + col : (size_t)col * nrows + row;
			if (idx >= n)
				continue;
			if (col == ncols - 1)
				printf("%s", cells[idx]);
			else
				printf("%-*s", (int)(maxw + 2), cells[idx]);
		}
		printf("\n");
	}

	for (i = 0; i < n; i++)
		free(cells[i]);
	free(cells);
}

void
printentries(struct entry *ents, size_t n, int withtotal)
{
	if (n == 0)
		return;

	if (withtotal && (lflag || nflag || gflag || oflag || sflag))
		printtotal(ents, n);

	switch (outfmt) {
	case FMT_LONG:
		printlong(ents, n);
		break;
	case FMT_STREAM:
		printstream(ents, n);
		break;
	case FMT_COLS:
		printcols(ents, n, 0);
		break;
	case FMT_ACROSS:
		printcols(ents, n, 1);
		break;
	case FMT_ONE:
		printone(ents, n);
		break;
	default:
		if (isatty(STDOUT_FILENO))
			printcols(ents, n, 0);
		else
			printone(ents, n);
	}
}
