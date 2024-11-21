/*
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mkdir.c	8.2 (Berkeley) 1/25/94";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>

int	build __P((char *));
void	usage __P((void));

#ifdef __linux__
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

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, exitval, oct, omode, pflag;
	mode_t *set;
	char *ep, *mode;

	pflag = 0;
	mode = NULL;
	while ((ch = getopt(argc, argv, "m:p")) != EOF)
		switch(ch) {
		case 'p':
			pflag = 1;
			break;
		case 'm':
			mode = optarg;
			break;
		case '?':
		default:
			usage();
		}

	argc -= optind;
	argv += optind;
	if (argv[0] == NULL)
		usage();

	if (mode == NULL) {
		omode = S_IRWXU | S_IRWXG | S_IRWXO;
		oct = 1;
	} else if (*mode >= '0' && *mode <= '7') {
		omode = (int)strtol(mode, &ep, 8);
		if (omode < 0 || *ep)
			errx(1, "invalid file mode: %s", mode);
		oct = 1;
	} else {
		if ((set = setmode(mode)) == NULL)
			errx(1, "invalid file mode: %s", mode);
		oct = 0;
	}

	for (exitval = 0; *argv != NULL; ++argv) {
		if (pflag && build(*argv)) {
			exitval = 1;
			continue;
		}
		if (mkdir(*argv, oct ?
		    omode : getmode(set, S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
			warn("%s", *argv);
			exitval = 1;
		}
	}
	exit(exitval);
}

int
build(path)
	char *path;
{
	struct stat sb;
	mode_t numask, oumask;
	int first;
	char *p;

	p = path;
	if (p[0] == '/')		/* Skip leading '/'. */
		++p;
	for (first = 1;; ++p) {
		if (p[0] == '\0' || p[0] == '/' && p[1] == '\0')
			break;
		if (p[0] != '/')
			continue;
		*p = '\0';
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those cased by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(S_IWUSR | S_IXUSR);
			(void)umask(numask);
			first = 0;
		}
		if (stat(path, &sb)) {
			if (errno != ENOENT ||
			    mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
				warn("%s", path);
				return (1);
			}
		}
		*p = '/';
	}
	if (!first)
		(void)umask(oumask);
	return (0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: mkdir [-p] [-m mode] directory ...\n");
	exit (1);
}
