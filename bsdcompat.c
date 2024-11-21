/* $Id$ */

#include <sys/stat.h>
#include <string.h>
#include <err.h>
#include <pwd.h>
#include <grp.h>
#include <stddef.h>
#include <signal.h>
#include <ctype.h>
#include <limits.h>

#include "datedata.h"

#define QUAD_MAX 0x7fffffffffffffffLL
#define S_IFWHT 0
#define S_ISWHT(x) 0
#define undelete remove
#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE '\0'
#endif
#define TTYDISC 0
#define SLIPDISC 1
#define TABLDISC 2
#define ALTWERASE 0

#ifndef BSDCOMPAT_IMPLEMENTATION
char* user_from_uid(uid_t uid, int nouser);
char* group_from_gid(gid_t gid, int nouser);
char* getbsize(int* headerlenp, long* blocksizep);
void strmode(mode_t mode, char *p);
int setpassent(int stayopen);
int setgroupent(int stayopen);
void* setmode(const char *p);
mode_t getmode(const void *bbox, mode_t omode);
extern const char *const sys_signame[];
#else
char* user_from_uid(uid, nouser)
	uid_t uid;
	int nouser;
{
	struct passwd* pwd = getpwuid(uid);
	if(pwd == NULL) return NULL;
	return pwd->pw_name;
}

char* group_from_gid(gid, nouser)
	gid_t gid;
	int nouser;
{
	struct group* grp = getgrgid(gid);
	if(grp == NULL) return NULL;
	return grp->gr_name;
}

char *
getbsize(headerlenp, blocksizep)
        int *headerlenp;
        long *blocksizep;
{
        static char header[20];
        long n, max, mul, blocksize;
        char *ep, *p, *form;

#define KB      (1024L)
#define MB      (1024L * 1024L)
#define GB      (1024L * 1024L * 1024L)
#define MAXB    GB              /* No tera, peta, nor exa. */
        form = "";
        if ((p = getenv("BLOCKSIZE")) != NULL && *p != '\0') {
                if ((n = strtol(p, &ep, 10)) < 0)
                        goto underflow;
                if (n == 0)
                        n = 1;
                if (*ep && ep[1])
                        goto fmterr;
                switch (*ep) {
                case 'G': case 'g':
                        form = "G";
                        max = MAXB / GB;
                        mul = GB;
                        break;
                case 'K': case 'k':
                        form = "K";
                        max = MAXB / KB;
                        mul = KB;
                        break;
                case 'M': case 'm':
                        form = "M";
                        max = MAXB / MB;
                        mul = MB;
                        break;
                case '\0':
                        max = MAXB;
                        mul = 1;
                        break;
                default:
fmterr:                 warnx("%s: unknown blocksize", p);
                        n = 512;
                        mul = 1;
                        break;
                }
                if (n > max) {
                        warnx("maximum blocksize is %dG", MAXB / GB);
                        n = max;
		}
                if ((blocksize = n * mul) < 512) {
underflow:              warnx("minimum blocksize is 512");
                        form = "";
                        blocksize = n = 512;
                }
        } else
                blocksize = n = 512;

        (void)snprintf(header, sizeof(header), "%d%s-blocks", n, form);
        *headerlenp = strlen(header);
        *blocksizep = blocksize;
        return (header);
}

void
strmode(mode_t mode, char *p)
{
	 /* print type */
	switch (mode & S_IFMT) {
	case S_IFDIR:			/* directory */
		*p++ = 'd';
		break;
	case S_IFCHR:			/* character special */
		*p++ = 'c';
		break;
	case S_IFBLK:			/* block special */
		*p++ = 'b';
		break;
	case S_IFREG:			/* regular */
		*p++ = '-';
		break;
	case S_IFLNK:			/* symbolic link */
		*p++ = 'l';
		break;
	case S_IFSOCK:			/* socket */
		*p++ = 's';
		break;
#ifdef S_IFIFO
	case S_IFIFO:			/* fifo */
		*p++ = 'p';
		break;
#endif
#ifdef S_IFWHT
	case S_IFWHT:			/* whiteout */
		*p++ = 'w';
		break;
#endif
	default:			/* unknown */
		*p++ = '?';
		break;
	}
	/* usr */
	if (mode & S_IRUSR)
		*p++ = 'r';
	else
		*p++ = '-';
	if (mode & S_IWUSR)
		*p++ = 'w';
	else
		*p++ = '-';
	switch (mode & (S_IXUSR | S_ISUID)) {
	case 0:
		*p++ = '-';
		break;
	case S_IXUSR:
		*p++ = 'x';
		break;
	case S_ISUID:
		*p++ = 'S';
		break;
	case S_IXUSR | S_ISUID:
		*p++ = 's';
		break;
	}
	/* group */
	if (mode & S_IRGRP)
		*p++ = 'r';
	else
		*p++ = '-';
	if (mode & S_IWGRP)
		*p++ = 'w';
	else
		*p++ = '-';
	switch (mode & (S_IXGRP | S_ISGID)) {
	case 0:
		*p++ = '-';
		break;
	case S_IXGRP:
		*p++ = 'x';
		break;
	case S_ISGID:
		*p++ = 'S';
		break;
	case S_IXGRP | S_ISGID:
		*p++ = 's';
		break;
	}
	/* other */
	if (mode & S_IROTH)
		*p++ = 'r';
	else
		*p++ = '-';
	if (mode & S_IWOTH)
		*p++ = 'w';
	else
		*p++ = '-';
	switch (mode & (S_IXOTH | S_ISVTX)) {
	case 0:
		*p++ = '-';
		break;
	case S_IXOTH:
		*p++ = 'x';
		break;
	case S_ISVTX:
		*p++ = 'T';
		break;
	case S_IXOTH | S_ISVTX:
		*p++ = 't';
		break;
	}
	*p++ = ' ';		/* will be a '+' if ACL's implemented */
	*p = '\0';
}

const char *const sys_signame[] = {
        "HUP",
        "INT",
        "QUIT",
        "ILL",
        "TRAP",
        "ABRT",
        "IOT",
        "BUS",
        "FPE",
        "KILL",
        "USR",
        "SEGV",
        "USR",
        "PIPE",
        "ALRM",
        "TERM",
        "STKFLT",
        "CHLD",
        "CONT",
        "STOP",
        "TSTP",
        "TTIN",
        "TTOU",
        "URG",
        "XCPU",
        "XFSZ",
        "VTALRM",
        "PROF",
        "WINCH",
        "IO",
        "POLL",
        "LOST",
        "PWR",
        "SYS",
        "RTMIN",
        "RTMAX",
        "STKSZ",
        "RT0",
        "RT1",
        "RT2",
        "RT3",
        "RT4",
        "RT5",
        "RT6",
        "RT7",
        "RT8",
        "RT9",
        "RT10",
        "RT11",
        "RT12",
        "RT13",
        "RT14",
        "RT15",
        "RT16",
        "RT17",
        "RT18",
        "RT19",
        "RT20",
        "RT21",
        "RT22",
        "RT23",
        "RT24",
        "RT25",
        "RT26",
        "RT27"
};

int setpassent(int stayopen){
	setpwent();
}

int setgroupent(int stayopen){
	setgrent();
}

#define S_ISTXT 0
#define	SET_LEN	6		/* initial # of bitcmd struct to malloc */
#define	SET_LEN_INCR 4		/* # of bitcmd structs to add as needed */

typedef struct bitcmd {
	char	cmd;
	char	cmd2;
	mode_t	bits;
} BITCMD;

#define	CMD2_CLR	0x01
#define	CMD2_SET	0x02
#define	CMD2_GBITS	0x04
#define	CMD2_OBITS	0x08
#define	CMD2_UBITS	0x10

static BITCMD	*addcmd(BITCMD *, mode_t, mode_t, mode_t, mode_t);
static void	 compress_mode(BITCMD *);

mode_t
getmode(const void *bbox, mode_t omode)
{
	const BITCMD *set;
	mode_t clrval, newmode, value;

	set = (const BITCMD *)bbox;
	newmode = omode;
	for (value = 0;; set++)
		switch(set->cmd) {
		/*
		 * When copying the user, group or other bits around, we "know"
		 * where the bits are in the mode so that we can do shifts to
		 * copy them around.  If we don't use shifts, it gets real
		 * grundgy with lots of single bit checks and bit sets.
		 */
		case 'u':
			value = (newmode & S_IRWXU) >> 6;
			goto common;

		case 'g':
			value = (newmode & S_IRWXG) >> 3;
			goto common;

		case 'o':
			value = newmode & S_IRWXO;
common:			if (set->cmd2 & CMD2_CLR) {
				clrval =
				    (set->cmd2 & CMD2_SET) ?  S_IRWXO : value;
				if (set->cmd2 & CMD2_UBITS)
					newmode &= ~((clrval<<6) & set->bits);
				if (set->cmd2 & CMD2_GBITS)
					newmode &= ~((clrval<<3) & set->bits);
				if (set->cmd2 & CMD2_OBITS)
					newmode &= ~(clrval & set->bits);
			}
			if (set->cmd2 & CMD2_SET) {
				if (set->cmd2 & CMD2_UBITS)
					newmode |= (value<<6) & set->bits;
				if (set->cmd2 & CMD2_GBITS)
					newmode |= (value<<3) & set->bits;
				if (set->cmd2 & CMD2_OBITS)
					newmode |= value & set->bits;
			}
			break;

		case '+':
			newmode |= set->bits;
			break;

		case '-':
			newmode &= ~set->bits;
			break;

		case 'X':
			if (omode & (S_IFDIR|S_IXUSR|S_IXGRP|S_IXOTH))
				newmode |= set->bits;
			break;

		case '\0':
		default:
#ifdef SETMODE_DEBUG
			(void)printf("getmode:%04o -> %04o\n", omode, newmode);
#endif
			return (newmode);
		}
}

#define	ADDCMD(a, b, c, d) do {						\
	if (set >= endset) {						\
		ptrdiff_t setdiff = set - saveset;			\
		BITCMD *newset;						\
		setlen += SET_LEN_INCR;					\
		newset = reallocarray(saveset, setlen, sizeof(BITCMD));	\
		if (newset == NULL)					\
			goto out;					\
		set = newset + setdiff;					\
		saveset = newset;					\
		endset = newset + (setlen - 2);				\
	}								\
	set = addcmd(set, (mode_t)(a), (mode_t)(b), (mode_t)(c), (d));	\
} while (/*CONSTCOND*/0)

#define	STANDARD_BITS	(S_ISUID|S_ISGID|S_IRWXU|S_IRWXG|S_IRWXO)

void *
setmode(const char *p)
{
	int serrno;
	char op, *ep;
	BITCMD *set, *saveset, *endset;
	sigset_t signset, sigoset;
	mode_t mask, perm, permXbits, who;
	long lval;
	int equalopdone = 0;	/* pacify gcc */
	int setlen;

	if (!*p) {
		errno = EINVAL;
		return NULL;
	}

	/*
	 * Get a copy of the mask for the permissions that are mask relative.
	 * Flip the bits, we want what's not set.  Since it's possible that
	 * the caller is opening files inside a signal handler, protect them
	 * as best we can.
	 */
	sigfillset(&signset);
	(void)sigprocmask(SIG_BLOCK, &signset, &sigoset);
	(void)umask(mask = umask(0));
	mask = ~mask;
	(void)sigprocmask(SIG_SETMASK, &sigoset, NULL);

	setlen = SET_LEN + 2;

	set = reallocarray(NULL, setlen, sizeof(BITCMD));
	if (set == NULL)
		return (NULL);
	saveset = set;
	endset = set + (setlen - 2);

	/*
	 * If an absolute number, get it and return; disallow non-octal digits
	 * or illegal bits.
	 */
	if (isdigit((unsigned char)*p)) {
		errno = 0;
		lval = strtol(p, &ep, 8);
		if (*ep) {
			errno = EINVAL;
			goto out;
		}
		if (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN))
			goto out;
		if (lval & ~(STANDARD_BITS|S_ISTXT)) {
			errno = EINVAL;
			goto out;
		}
		perm = (mode_t)lval;
		ADDCMD('=', (STANDARD_BITS|S_ISTXT), perm, mask);
		set->cmd = 0;
		return (saveset);
	}

	/*
	 * Build list of structures to set/clear/copy bits as described by
	 * each clause of the symbolic mode.
	 */
	for (;;) {
		/* First, find out which bits might be modified. */
		for (who = 0;; ++p) {
			switch (*p) {
			case 'a':
				who |= STANDARD_BITS;
				break;
			case 'u':
				who |= S_ISUID|S_IRWXU;
				break;
			case 'g':
				who |= S_ISGID|S_IRWXG;
				break;
			case 'o':
				who |= S_IRWXO;
				break;
			default:
				goto getop;
			}
		}

getop:		if ((op = *p++) != '+' && op != '-' && op != '=') {
			errno = EINVAL;
			goto out;
		}
		if (op == '=')
			equalopdone = 0;

		who &= ~S_ISTXT;
		for (perm = 0, permXbits = 0;; ++p) {
			switch (*p) {
			case 'r':
				perm |= S_IRUSR|S_IRGRP|S_IROTH;
				break;
			case 's':
				/*
				 * If specific bits where requested and
				 * only "other" bits ignore set-id.
				 */
				if (who == 0 || (who & ~S_IRWXO))
					perm |= S_ISUID|S_ISGID;
				break;
			case 't':
				/*
				 * If specific bits where requested and
				 * only "other" bits ignore set-id.
				 */
				if (who == 0 || (who & ~S_IRWXO)) {
					who |= S_ISTXT;
					perm |= S_ISTXT;
				}
				break;
			case 'w':
				perm |= S_IWUSR|S_IWGRP|S_IWOTH;
				break;
			case 'X':
				permXbits = S_IXUSR|S_IXGRP|S_IXOTH;
				break;
			case 'x':
				perm |= S_IXUSR|S_IXGRP|S_IXOTH;
				break;
			case 'u':
			case 'g':
			case 'o':
				/*
				 * When ever we hit 'u', 'g', or 'o', we have
				 * to flush out any partial mode that we have,
				 * and then do the copying of the mode bits.
				 */
				if (perm) {
					ADDCMD(op, who, perm, mask);
					perm = 0;
				}
				if (op == '=')
					equalopdone = 1;
				if (op == '+' && permXbits) {
					ADDCMD('X', who, permXbits, mask);
					permXbits = 0;
				}
				ADDCMD(*p, who, op, mask);
				break;

			default:
				/*
				 * Add any permissions that we haven't already
				 * done.
				 */
				if (perm || (op == '=' && !equalopdone)) {
					if (op == '=')
						equalopdone = 1;
					ADDCMD(op, who, perm, mask);
					perm = 0;
				}
				if (permXbits) {
					ADDCMD('X', who, permXbits, mask);
					permXbits = 0;
				}
				goto apply;
			}
		}

apply:		if (!*p)
			break;
		if (*p != ',')
			goto getop;
		++p;
	}
	set->cmd = 0;
#ifdef SETMODE_DEBUG
	(void)printf("Before compress_mode()\n");
	dumpmode(saveset);
#endif
	compress_mode(saveset);
#ifdef SETMODE_DEBUG
	(void)printf("After compress_mode()\n");
	dumpmode(saveset);
#endif
	return (saveset);
out:
	serrno = errno;
	free(saveset);
	errno = serrno;
	return NULL;
}

static BITCMD *
addcmd(BITCMD *set, mode_t op, mode_t who, mode_t oparg, mode_t mask)
{
	switch (op) {
	case '=':
		set->cmd = '-';
		set->bits = who ? who : STANDARD_BITS;
		set++;

		op = '+';
		/* FALLTHROUGH */
	case '+':
	case '-':
	case 'X':
		set->cmd = op;
		set->bits = (who ? who : mask) & oparg;
		break;

	case 'u':
	case 'g':
	case 'o':
		set->cmd = op;
		if (who) {
			set->cmd2 = ((who & S_IRUSR) ? CMD2_UBITS : 0) |
				    ((who & S_IRGRP) ? CMD2_GBITS : 0) |
				    ((who & S_IROTH) ? CMD2_OBITS : 0);
			set->bits = (mode_t)~0;
		} else {
			set->cmd2 = CMD2_UBITS | CMD2_GBITS | CMD2_OBITS;
			set->bits = mask;
		}

		if (oparg == '+')
			set->cmd2 |= CMD2_SET;
		else if (oparg == '-')
			set->cmd2 |= CMD2_CLR;
		else if (oparg == '=')
			set->cmd2 |= CMD2_SET|CMD2_CLR;
		break;
	}
	return (set + 1);
}

static void
compress_mode(BITCMD *set)
{
	BITCMD *nset;
	int setbits, clrbits, Xbits, op;

	for (nset = set;;) {
		/* Copy over any 'u', 'g' and 'o' commands. */
		while ((op = nset->cmd) != '+' && op != '-' && op != 'X') {
			*set++ = *nset++;
			if (!op)
				return;
		}

		for (setbits = clrbits = Xbits = 0;; nset++) {
			if ((op = nset->cmd) == '-') {
				clrbits |= nset->bits;
				setbits &= ~nset->bits;
				Xbits &= ~nset->bits;
			} else if (op == '+') {
				setbits |= nset->bits;
				clrbits &= ~nset->bits;
				Xbits &= ~nset->bits;
			} else if (op == 'X')
				Xbits |= nset->bits & ~setbits;
			else
				break;
		}
		if (clrbits) {
			set->cmd = '-';
			set->cmd2 = 0;
			set->bits = clrbits;
			set++;
		}
		if (setbits) {
			set->cmd = '+';
			set->cmd2 = 0;
			set->bits = setbits;
			set++;
		}
		if (Xbits) {
			set->cmd = 'X';
			set->cmd2 = 0;
			set->bits = Xbits;
			set++;
		}
	}
}
#endif
