/* $Id$ */

#include <sys/stat.h>
#include <string.h>
#include <err.h>
#include <pwd.h>
#include <grp.h>

#define SECSPERMIN      60
#define MINSPERHOUR     60
#define HOURSPERDAY     24
#define DAYSPERWEEK     7
#define DAYSPERNYEAR    365
#define DAYSPERLYEAR    366
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      ((int) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR     12
#define QUAD_MAX 0x7fffffffffffffffLL

#ifndef BSDCOMPAT_IMPLEMENTATION
char* user_from_uid(uid_t uid, int nouser);
char* group_from_gid(gid_t gid, int nouser);
char* getbsize(int* headerlenp, long* blocksizep);
void strmode(mode_t mode, char *p);
int setpassent(int stayopen);
int setgroupent(int stayopen);
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
#endif
