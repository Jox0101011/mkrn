/* touch - change file access and modification times (POSIX.1-2008) */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static char *argv0;
static int status;

static void
usage(void)
{
	fprintf(stderr,
	    "usage: %s [-acm] [-r ref_file | -t time | -d date_time] file...\n",
	    argv0);
	exit(1);
}

static int
digits(const char *s, int n)
{
	int i;

	for (i = 0; i < n; i++)
		if (!isdigit((unsigned char)s[i]))
			return 0;
	return 1;
}

/* -t [[CC]YY]MMDDhhmm[.SS] */
static int
parsettime(const char *s, struct timespec *ts)
{
	size_t len = strcspn(s, ".");
	const char *p = s;
	int cc, yy, year, mon, day, hh, mm, ss = 0;
	struct tm tmv;
	time_t t;

	if ((len != 8 && len != 10 && len != 12) || !digits(s, (int)len))
		return -1;
	if (s[len] == '.') {
		if (strlen(s + len + 1) != 2 || !digits(s + len + 1, 2))
			return -1;
		ss = (s[len + 1] - '0') * 10 + (s[len + 2] - '0');
	} else if (s[len] != '\0') {
		return -1;
	}

	if (len == 8) {
		struct tm nowtm;
		time_t now = time(NULL);

		localtime_r(&now, &nowtm);
		year = nowtm.tm_year + 1900;
	} else if (len == 10) {
		yy = (p[0] - '0') * 10 + (p[1] - '0');
		cc = (yy >= 69) ? 19 : 20;
		year = cc * 100 + yy;
		p += 2;
	} else {
		cc = (p[0] - '0') * 10 + (p[1] - '0');
		yy = (p[2] - '0') * 10 + (p[3] - '0');
		year = cc * 100 + yy;
		p += 4;
	}

	mon = (p[0] - '0') * 10 + (p[1] - '0');
	day = (p[2] - '0') * 10 + (p[3] - '0');
	hh  = (p[4] - '0') * 10 + (p[5] - '0');
	mm  = (p[6] - '0') * 10 + (p[7] - '0');
	if (mon < 1 || mon > 12 || day < 1 || day > 31 || hh > 23 || mm > 59 || ss > 60)
		return -1;

	memset(&tmv, 0, sizeof tmv);
	tmv.tm_year = year - 1900;
	tmv.tm_mon = mon - 1;
	tmv.tm_mday = day;
	tmv.tm_hour = hh;
	tmv.tm_min = mm;
	tmv.tm_sec = ss;
	tmv.tm_isdst = -1;
	if ((t = mktime(&tmv)) == (time_t)-1)
		return -1;
	ts->tv_sec = t;
	ts->tv_nsec = 0;
	return 0;
}

/* -d YYYY-MM-DDThh:mm:SS[.frac][Z], T may be a space, . may be a comma */
static int
parsedtime(const char *s, struct timespec *ts)
{
	const char *p = s;
	char *end;
	int year, mon, day, hh, mm, ss, utc = 0;
	long nsec = 0;
	size_t ndig;
	struct tm tmv;
	time_t t;

	if (strspn(p, "0123456789") < 4)
		return -1;
	year = (int)strtol(p, &end, 10);
	p = end;

	if (*p != '-' || !digits(p + 1, 2))
		return -1;
	mon = (p[1] - '0') * 10 + (p[2] - '0');
	p += 3;
	if (*p != '-' || !digits(p + 1, 2))
		return -1;
	day = (p[1] - '0') * 10 + (p[2] - '0');
	p += 3;

	if ((*p != 'T' && *p != ' ') || !digits(p + 1, 2))
		return -1;
	hh = (p[1] - '0') * 10 + (p[2] - '0');
	p += 3;
	if (*p != ':' || !digits(p + 1, 2))
		return -1;
	mm = (p[1] - '0') * 10 + (p[2] - '0');
	p += 3;
	if (*p != ':' || !digits(p + 1, 2))
		return -1;
	ss = (p[1] - '0') * 10 + (p[2] - '0');
	p += 3;

	if (*p == '.' || *p == ',') {
		char buf[10];
		size_t k;

		p++;
		if ((ndig = strspn(p, "0123456789")) == 0)
			return -1;
		for (k = 0; k < 9; k++)
			buf[k] = (k < ndig) ? p[k] : '0';
		buf[9] = '\0';
		nsec = strtol(buf, NULL, 10);
		p += ndig;
	}
	if (*p == 'Z') {
		utc = 1;
		p++;
	}
	if (*p != '\0')
		return -1;
	if (mon < 1 || mon > 12 || day < 1 || day > 31 || hh > 23 || mm > 59 || ss > 60)
		return -1;

	memset(&tmv, 0, sizeof tmv);
	tmv.tm_year = year - 1900;
	tmv.tm_mon = mon - 1;
	tmv.tm_mday = day;
	tmv.tm_hour = hh;
	tmv.tm_min = mm;
	tmv.tm_sec = ss;

	if (utc) {
		/* no timegm() in POSIX base: force TZ=UTC0 around mktime() */
		char *old = getenv("TZ");
		char *saved = old ? strdup(old) : NULL;

		setenv("TZ", "UTC0", 1);
		tzset();
		tmv.tm_isdst = 0;
		t = mktime(&tmv);
		if (saved != NULL) {
			setenv("TZ", saved, 1);
			free(saved);
		} else {
			unsetenv("TZ");
		}
		tzset();
	} else {
		tmv.tm_isdst = -1;
		t = mktime(&tmv);
	}
	if (t == (time_t)-1)
		return -1;
	ts->tv_sec = t;
	ts->tv_nsec = nsec;
	return 0;
}

static struct timespec
omit(void)
{
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = UTIME_OMIT;
	return ts;
}

int
main(int argc, char *argv[])
{
	int ch, i, fd, aflag = 0, mflag = 0, cflag = 0, userref = 0;
	char srcmode = 0;
	char *refname = NULL, *timearg = NULL, *datearg = NULL;
	struct timespec srctime, reftime_a = {0}, reftime_m = {0};

	srctime.tv_sec = 0;
	srctime.tv_nsec = UTIME_NOW;

	argv0 = argv[0];
	while ((ch = getopt(argc, argv, "acmr:t:d:")) != -1) {
		switch (ch) {
		case 'a': aflag = 1; break;
		case 'c': cflag = 1; break;
		case 'm': mflag = 1; break;
		case 'r': srcmode = 'r'; refname = optarg; break;
		case 't': srcmode = 't'; timearg = optarg; break;
		case 'd': srcmode = 'd'; datearg = optarg; break;
		default: usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc == 0)
		usage();

	if (!aflag && !mflag)
		aflag = mflag = 1;

	if (srcmode == 'r') {
		struct stat rst;

		if (stat(refname, &rst) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, refname, strerror(errno));
			exit(1);
		}
		reftime_a = rst.st_atim;
		reftime_m = rst.st_mtim;
		userref = 1;
	} else if (srcmode == 't') {
		if (parsettime(timearg, &srctime) < 0) {
			fprintf(stderr, "%s: invalid time '%s'\n", argv0, timearg);
			exit(1);
		}
	} else if (srcmode == 'd') {
		if (parsedtime(datearg, &srctime) < 0) {
			fprintf(stderr, "%s: invalid date_time '%s'\n", argv0, datearg);
			exit(1);
		}
	}

	for (i = 0; i < argc; i++) {
		struct timespec times[2];
		struct stat st;

		times[0] = aflag ? (userref ? reftime_a : srctime) : omit();
		times[1] = mflag ? (userref ? reftime_m : srctime) : omit();

		if (stat(argv[i], &st) < 0) {
			if (cflag)
				continue;
			if ((fd = open(argv[i], O_WRONLY | O_CREAT,
			    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0) {
				fprintf(stderr, "%s: %s: %s\n", argv0, argv[i], strerror(errno));
				status = 1;
				continue;
			}
			if (futimens(fd, times) < 0) {
				fprintf(stderr, "%s: %s: %s\n", argv0, argv[i], strerror(errno));
				status = 1;
			}
			close(fd);
		} else if (utimensat(AT_FDCWD, argv[i], times, 0) < 0) {
			fprintf(stderr, "%s: %s: %s\n", argv0, argv[i], strerror(errno));
			status = 1;
		}
	}

	return status;
}
