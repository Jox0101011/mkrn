/* cp - copy files (POSIX.1-2008) */
/* mknod() is XSI, needed only to recreate device nodes with -R since there
 * is no base-POSIX way to do so. every other function used here is base */
#define _XOPEN_SOURCE 500
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

enum { SYM_UNSET, SYM_H, SYM_L, SYM_P };

static char *argv0;
static int fflag, iflag, pflag, Rflag, symopt, status;

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-Pfip] source_file target_file\n"
	    "       %s [-Pfip] source_file... target\n"
	    "       %s -R [-H|-L|-P] [-fip] source_file... target\n",
	    argv0, argv0, argv0);
	exit(1);
}

/* whether to follow (stat) a symlink at this depth; 0 is a top-level operand */
static int
follow(int depth)
{
	switch (symopt) {
	case SYM_P:
		return 0;
	case SYM_L:
		return 1;
	case SYM_H:
		return depth == 0;
	default:
		return Rflag ? 0 : 1;
	}
}

static int
confirm(const char *dest)
{
	int c, yes;

	fprintf(stderr, "%s: overwrite '%s'? ", argv0, dest);
	yes = (c = getchar()) == 'y' || c == 'Y';
	while (c != '\n' && c != EOF)
		c = getchar();
	return yes;
}

static mode_t
getumask(void)
{
	mode_t u = umask(0);

	umask(u);
	return u;
}

static char *
joinpath(const char *dir, const char *name)
{
	size_t dn;
	char *p;

	dn = strlen(dir);
	while (dn > 1 && dir[dn - 1] == '/')
		dn--;
	if ((p = malloc(dn + 1 + strlen(name) + 1)) == NULL) {
		fprintf(stderr, "%s: out of memory\n", argv0);
		exit(1);
	}
	memcpy(p, dir, dn);
	p[dn] = '/';
	strcpy(p + dn + 1, name);
	return p;
}

static char *
destname(const char *target, const char *src)
{
	const char *b = strrchr(src, '/');

	return joinpath(target, b ? b + 1 : src);
}

/* true if a and b, symlinks resolved, name the same file */
static int
samefile(const char *a, const char *b)
{
	struct stat sa, sb;

	return stat(a, &sa) == 0 && stat(b, &sb) == 0 &&
	    sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

/* -p: duplicate mtime/atime, owner and permissions of src onto dest */
static void
preserve(const char *src, const char *dest, struct stat *sst)
{
	struct utimbuf ut;
	mode_t mode = sst->st_mode & 07777;

	ut.actime = sst->st_atime;
	ut.modtime = sst->st_mtime;
	if (utime(dest, &ut) < 0)
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));

	/* owner/group could not be duplicated: drop setuid/setgid */
	if (chown(dest, sst->st_uid, sst->st_gid) < 0)
		mode &= ~(S_ISUID | S_ISGID);
	if (chmod(dest, mode) < 0)
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
}

static void
copy_regular(const char *src, const char *dest, struct stat *sst, int destexists)
{
	int sfd, dfd;
	mode_t mode = sst->st_mode & 07777;
	char buf[BUFSIZ];
	ssize_t n, w, off;

	if (destexists && iflag && !confirm(dest))
		return;

	if ((sfd = open(src, O_RDONLY)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, src, strerror(errno));
		status = 1;
		return;
	}

	if (destexists) {
		dfd = open(dest, O_WRONLY | O_TRUNC);
		if (dfd < 0 && fflag && unlink(dest) == 0)
			dfd = open(dest, O_WRONLY | O_CREAT, mode);
	} else {
		dfd = open(dest, O_WRONLY | O_CREAT, mode);
	}
	if (dfd < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
		status = 1;
		close(sfd);
		return;
	}

	while ((n = read(sfd, buf, sizeof buf)) > 0) {
		for (off = 0; off < n; off += w) {
			if ((w = write(dfd, buf + off, n - off)) < 0) {
				fprintf(stderr, "%s: %s: %s\n",
				    argv0, dest, strerror(errno));
				status = 1;
				close(sfd);
				close(dfd);
				return;
			}
		}
	}
	if (n < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, src, strerror(errno));
		status = 1;
	}
	close(sfd);
	close(dfd);

	if (pflag)
		preserve(src, dest, sst);
}

static void
copy_symlink(const char *src, const char *dest, int destexists)
{
	char buf[PATH_MAX];
	ssize_t n;

	if ((n = readlink(src, buf, sizeof buf - 1)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, src, strerror(errno));
		status = 1;
		return;
	}
	buf[n] = '\0';

	if (destexists && unlink(dest) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
		status = 1;
		return;
	}
	if (symlink(buf, dest) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
		status = 1;
	}
}

/* -R: recreate a fifo or device node. mkfifo() is POSIX base; a device node
 * has no base-POSIX equivalent, so mknod() (XSI) is used for that case only */
static void
copy_special(const char *src, const char *dest, struct stat *sst, int destexists)
{
	mode_t mode = sst->st_mode & 07777;

	if (destexists && unlink(dest) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
		status = 1;
		return;
	}

	if (S_ISFIFO(sst->st_mode)) {
		if (mkfifo(dest, mode) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
			status = 1;
			return;
		}
	} else if (S_ISBLK(sst->st_mode) || S_ISCHR(sst->st_mode)) {
		if (mknod(dest, sst->st_mode, sst->st_rdev) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
			status = 1;
			return;
		}
	} else {
		fprintf(stderr, "%s: %s: unsupported file type\n", argv0, src);
		status = 1;
		return;
	}

	if (pflag)
		preserve(src, dest, sst);
}

static void copy_source(const char *src, const char *dest, int depth);

static void
copy_dir(const char *src, const char *dest, struct stat *sst, int destexists)
{
	struct stat dst;
	DIR *dp;
	struct dirent *de;
	char *s2, *d2;
	int created = 0;
	mode_t mode;

	if (destexists) {
		if (stat(dest, &dst) < 0 || !S_ISDIR(dst.st_mode)) {
			fprintf(stderr, "%s: %s: not a directory\n", argv0, dest);
			status = 1;
			return;
		}
	} else {
		if (mkdir(dest, (sst->st_mode & 07777) | S_IRWXU) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, dest, strerror(errno));
			status = 1;
			return;
		}
		created = 1;
	}

	if ((dp = opendir(src)) == NULL) {
		fprintf(stderr, "%s: %s: %s\n", argv0, src, strerror(errno));
		status = 1;
		return;
	}
	while ((de = readdir(dp)) != NULL) {
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
		s2 = joinpath(src, de->d_name);
		d2 = joinpath(dest, de->d_name);
		copy_source(s2, d2, 1);
		free(s2);
		free(d2);
	}
	closedir(dp);

	if (created) {
		if (pflag) {
			preserve(src, dest, sst);
		} else {
			mode = (sst->st_mode & 07777) & ~getumask();
			if (chmod(dest, mode) < 0)
				fprintf(stderr, "%s: %s: %s\n",
				    argv0, dest, strerror(errno));
		}
	}
}

/* copy one source (file, directory or, with -R, other types) to dest */
static void
copy_source(const char *src, const char *dest, int depth)
{
	struct stat sst, dtmp;
	int destexists, followsrc = follow(depth);

	if ((followsrc ? stat(src, &sst) : lstat(src, &sst)) < 0) {
		fprintf(stderr, "%s: %s: %s\n", argv0, src, strerror(errno));
		status = 1;
		return;
	}

	destexists = lstat(dest, &dtmp) == 0;

	if (destexists && samefile(src, dest)) {
		fprintf(stderr, "%s: '%s' and '%s' are the same file\n",
		    argv0, src, dest);
		status = 1;
		return;
	}

	if (S_ISDIR(sst.st_mode)) {
		if (!Rflag) {
			fprintf(stderr, "%s: %s: is a directory\n", argv0, src);
			status = 1;
			return;
		}
		copy_dir(src, dest, &sst, destexists);
	} else if (S_ISREG(sst.st_mode)) {
		copy_regular(src, dest, &sst, destexists);
	} else if (S_ISLNK(sst.st_mode) && Rflag) {
		copy_symlink(src, dest, destexists);
	} else if (Rflag) {
		copy_special(src, dest, &sst, destexists);
	} else {
		fprintf(stderr, "%s: %s: not a regular file (use -R)\n", argv0, src);
		status = 1;
	}
}

int
main(int argc, char *argv[])
{
	int ch, i, nsrc, targetisdir;
	char *target, *dest;
	struct stat tst;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "HLPRfip")) != -1) {
		switch (ch) {
		case 'H':
			symopt = SYM_H;
			break;
		case 'L':
			symopt = SYM_L;
			break;
		case 'P':
			symopt = SYM_P;
			break;
		case 'R':
			Rflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'i':
			iflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	nsrc = argc - 1;
	target = argv[nsrc];
	targetisdir = stat(target, &tst) == 0 && S_ISDIR(tst.st_mode);

	if (nsrc > 1 && !targetisdir) {
		fprintf(stderr, "%s: %s: not a directory\n", argv0, target);
		exit(1);
	}

	if (nsrc == 1 && !targetisdir) {
		copy_source(argv[0], target, 0);
	} else {
		for (i = 0; i < nsrc; i++) {
			dest = destname(target, argv[i]);
			copy_source(argv[i], dest, 0);
			free(dest);
		}
	}

	return status;
}
