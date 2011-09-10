/**
 **	ESH version 1.7
 **
 **	The Environmental Meta Shell is intended as an intermediary between
 **	/bin/login and the user's real shell.  Its purpose is to set up a
 **	common environment for all users on a particular system that can
 **	easily be changed by the superuser as needed.
 **
 **	By default, esh will modify the already existing environment by
 **	reading bindings from /etc/environ before exec'ing the user's
 **	personal shell.  It is intended to replace the user's real shell
 **	in /etc/passwd and will take the real shell from the user's SHELL
 **	environment variable, or from the user's ~/.shell file.
 **
 **	Copyright (c) 1990-2011, Lennart Lovstrand <esh@lenlolabs.com>
 **
 **	First version: Tue Jan 16 19:16:16 1990
 **	Last edited: Thu Aug 25 02:11:15 2011
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/utsname.h>
#ifdef DEBUGTIME
#include <sys/types.h>
#include <sys/timeb.h>
#endif /* DEBUGTIME */
#include <pwd.h>

#define ESHVERSION	"1.7"

#define	SYSENVFILE	(ETCDIR "/environ")
#define USRENVFILE	"$HOME/.environ"
#define SYSSHELL	"${SHELL-/bin/csh}"
#define USRSHELL	"$HOME/.shell"
#define DEBUGFILE	"$HOME/.eshdebug"
#define BIGBUFSIZ	8192
#define ENVGROWTH	10
#define REARGSIZ	64
#define MAXKEYWORDS	64

#define TRUE		1
#define FALSE		0

#define NO_FORMAT	0
#define SH_FORMAT	1
#define CSH_FORMAT	2
#define LISP_FORMAT	3

enum editop {
    OP_DEFAULT,
    OP_REPLACE,
    OP_REMOVE,
    OP_APPEND,
};

extern char **environ;

char *readbinding(FILE *), *interpret(char *, int), *newstr(char *);
char **bassoc(const char *, char **), *mkbind(char *, char *);
void readenv(char *), fprintq(FILE *, char *), expand(char **, char **, int);
void compute(char **, char **, int), init_keywords(void), list_keywords(void);
void *xalloc(void *mem, long siz);
void editenv(enum editop op, const char *binding);

char **EnvBuf = NULL;
int EnvSiz = 0;

int Debug = FALSE; /* TRUE; */
char *SysEnvFile = SYSENVFILE;
char *UsrEnvFile = USRENVFILE;
char *Shell = NULL;
int ShellOut = NO_FORMAT;
int AutoPrunePaths = FALSE;

void
usage(name)
    char *name;
{
    fprintf(stderr, "usage: %s [-A args] [-B | -C] [-D] [-E sysenv] "
	    "[-F usrenv] [-L | -N] [-S shell] [shell-args...]\n", name);
    exit(1);
}

/* arg should be "[-A ]xxx yyy ..." */

void
rearg(int *pargc, char ***pargv, int *pargi)
{
    char *arg = (*pargv)[*pargi];
    char **nargv = (char **) xalloc(NULL, REARGSIZ * sizeof(char *));
    int argi = *pargi;
    int nargc;
    char *p = arg;

    for (nargc = 0; nargc < argi; nargc++)
	nargv[nargc] = (*pargv)[nargc];

    if (p[0] != '-' || p[1] != 'A') {
	/* no, first arg is real */
	nargv[nargc++] = p;
    }

    for (;;) {
	p = index(p + 1, ' ');

	if (p == NULL)
	    break;

	*p = '\0';
	nargv[nargc++] = p + 1;
    }

    while (++argi < *pargc)
	nargv[nargc++] = (*pargv)[argi];

    *pargc = nargc;
    *pargv = nargv;
}

char *argopt(int argc, char **argv, int *pargi)
{
    (*pargi)++;
    if (*pargi >= argc)
	usage(argv[0]);

    return argv[*pargi];
}

void printversion(void)
{
    printf("Version: " ESHVERSION "\n");
}

int procargs(int argc, char **argv)
{
    int argi;

    for (argi = 1; argi < argc && argv[argi][0] == '-'; argi++) {
	const char *opt;

	for (opt = &argv[argi][1]; *opt != '\0'; opt++) {
	    switch (*opt) {
	      case 'A': rearg(&argc, &argv, &argi); break;
	      case 'B': ShellOut = SH_FORMAT;	break;
	      case 'C': ShellOut = CSH_FORMAT;	break;
	      case 'D': Debug = !Debug;		break;
	      case 'E': SysEnvFile = argopt(argc, argv, &argi);	break;
	      case 'F': UsrEnvFile = argopt(argc, argv, &argi);	break;
	      case 'K': list_keywords(); exit(0); break;
	      case 'L': argv[0][0] = '-';	break;
	      case 'N': if (argv[0][0] == '-') argv[0][0] = 'x'; break;
	      case 'P': AutoPrunePaths = TRUE; break;
	      case 'S': Shell = argopt(argc, argv, &argi); break;
	      case 'V': printversion(); exit(0); break;
	      case 'X': ShellOut = LISP_FORMAT; break;
	      default:
		if (opt[-1] != '-')
		    usage(argv[0]);

		/* pass everything else on to shell */
		return argi;
	    }
	}
    }
    
    return argi;
}

void
printenv(char *var, char *val)
{
    switch (ShellOut) {
      case SH_FORMAT:
	printf("%s=", var);
	if (val != NULL) {
	    fprintq(stdout, val);
	}
	printf("; export %s\n", var);
	break;

      case CSH_FORMAT:
	printf("setenv %s", var);
	if (val != NULL) {
	    putchar(' ');
	    fprintq(stdout, val);
	}
	putchar('\n');
	break;

      case LISP_FORMAT:
	printf("(setenv \"%s\"", var);
	if (val != NULL) {
	    putchar(' ');
	    fprintq(stdout, val);
	}
	printf(")\n");
	break;
    }
}

int
main(argc, argv)
    int argc;
    char **argv;
{
    char **args, **ee, **oldenv = environ;
    char *p, buf[BUFSIZ];
    int argi;
    char *envflags;
#ifdef DEBUGTIME
    struct timeb before, after;

    (void) ftime(&before);
#endif /* DEBUGTIME */

    init_keywords();

    p = interpret(DEBUGFILE, FALSE);
    if (p != NULL && access(p, F_OK) == 0)
	Debug = TRUE;

    argi = procargs(argc, argv);

    if (Debug) {
	int a;
	fprintf(stderr, "args (i@%d):", argi);
	for (a = 0; a < argc; a++) {
	    if (a == argi)
		fprintf(stderr, " |");
	    fprintf(stderr, " [%d] %s", a, argv[a]);
	}
	fprintf(stderr, "\n");
    }

    /* copy old environment into a large enough buffer and replace it */
    for (ee = oldenv; *ee != NULL; ee++) {
	/* avoid infinite recursion -- ignore any SHELL=esh binding */
	if (strncmp(*ee, "SHELL=", 6) != 0) {
	    editenv(OP_APPEND, *ee);
	}
    }

    /* add global environment */
    if (Debug)
	printf("[--system environment--]\n");
    readenv(interpret(SysEnvFile, FALSE));

    /* add private environment */
    if (Debug)
	printf("[--user environment--]\n");
    readenv(interpret(UsrEnvFile, FALSE));

    /* reinterpret args in the environment (if any) */
    envflags = interpret(getenv("ESHFLAGS"), FALSE);
    if (envflags != NULL) {
	char *xargs[] = {argv[0], envflags, NULL};
	char **xargv = xargs;
	int xargc = 2;
	int xargi = 1;

	rearg(&xargc, &xargv, &xargi);
	(void) procargs(xargc, xargv);
    }

    /*
     * If the shell hasn't been given (normal case),
     * check ~/.shell to see what it should be.
     */
    if (Shell == NULL) {
	Shell = interpret(USRSHELL, FALSE);

	if (access(Shell, F_OK) < 0)
	    Shell = interpret(SYSSHELL, FALSE);

	if (access(Shell, X_OK) < 0) {
	    /* not executable, assume text file with name of shell */
	    FILE *stream = fopen(Shell, "r");

	    if (stream == NULL) {
		perror(Shell);
		exit(1);
	    }

	    if (fgets(buf, sizeof(buf), stream) == NULL) {
		perror(Shell);
		exit(1);
	    }

	    if ((p = index(buf, '\n')) != NULL)
		*p = '\0';

	    Shell = interpret(buf, FALSE);
	    (void) fclose(stream);
	}
    }

    /* rebind SHELL to point to the user-specified shell */
    if ((ee = bassoc("SHELL", environ)) == NULL) {
	*ee++ = mkbind("SHELL", Shell);
	*ee = NULL;
    } else {
	/* (void) free(*ee); -- if only we knew it was malloced */
	*ee =  mkbind("SHELL", Shell);
    }

    /*
     *  Only do shell source output?
     */
    if (ShellOut != NO_FORMAT) {
	for (ee = environ; *ee != NULL; ee++) {
	    /* Don't print out old bindings that we haven't changed */
	    for (; *oldenv != NULL; oldenv++) {
		const char *eq = strchr(*oldenv, '=');
		if (eq == NULL || strncmp(*ee, *oldenv, eq - *oldenv + 1) == 0)
		    break;
	    }
	    if (*oldenv != NULL && *oldenv++ == *ee)
		continue;
	    p = index(*ee, '=');
	    if (p == NULL) {
		printenv(*ee, NULL);
	    } else {
		*p = '\0';
		printenv(*ee, p + 1);
		*p = '=';
	    }
	}

	exit(0);
    }

    /*
     * Set up args for exec'ing the new shell and replace args[0]
     * with a leading dash (if we're a login shell) and the name of the shell.
     */
    args = &argv[argi-1];
    if ((p = rindex(Shell, '/')) == NULL)
	p = Shell;
    /*
     * Hack Attack!  If the file begins with a dot (like .shell) and it is
     * a symlink, then replace it with the target file so that the shell
     * won't get confused about what it is
     */
    if (p[0] == '.' || (p[0] == '/' && p[1] == '.')) {
	int len = readlink(Shell, buf, sizeof(buf));

	if (len >= 0) {
	    if (len == sizeof(buf))
		len--;
	    buf[len] = '\0';
	    if ((p = rindex(buf, '/')) == NULL)
		p = buf;
	}
    }
    if (argv[0][0] == '-') {
	args[0] = newstr(p);
	args[0][0] = '-';
    } else if (p == Shell)
	args[0] = newstr(p);
    else
	args[0] = newstr(p + 1);

    /* the shell is dead, long live the shell! */
    if (Debug) {
	char **pp;
	printf("[exec %s:", Shell);
	for (pp = args; *pp != NULL; pp++)
	    printf(" %s", *pp);
	printf("]\n");
    }
#ifdef DEBUGTIME
    (void) ftime(&after);
    fprintf(stderr, "[esh: time = %5.3f]\n",
	    (after.time + after.millitm * 0.001) -
	    (before.time + before.millitm * 0.001));
#endif /* DEBUGTIME */
    execvp(Shell, args);
    perror(Shell);
    execve("/bin/sh", args, environ);
    perror("/bin/sh");
    exit(1);
}

void
readenv(file)
    char *file;
{
    FILE *stream;
    char *binding;
#ifdef DISABLE_NONINTERACTIVE_PS1
    int interactive = isatty(0);
#endif

    if (file == NULL)
	return;

    if (strcmp(file, "-") == 0)
	stream = stdin;
    else {
	stream = fopen(file, "r");
	if (stream == NULL)
	    return;
    }

    while ((binding = readbinding(stream)) != NULL) {
	enum editop op = OP_REPLACE;

	if (Debug)
	    fprintf(stderr, "[%s]\n", binding);

#ifdef DISABLE_NONINTERACTIVE_PS1
	if (!interactive && strncmp(binding, "PS1=", 4) == 0)
	    op = OP_REMOVE;
#endif

	switch (*binding) {
	  case '\\':
	    binding++;
	    break;
	  case '-':
	    op = OP_REMOVE;
	    binding++;
	    break;
	  case '?':
	    op = OP_DEFAULT;
	    binding++;
	    break;
	}

	editenv(op, binding);
    }

    if (stream != stdin)
	(void) fclose(stream);
    return;
}

/*
 *	Make a freshly malloc'ed environment variable binding.
 *	mkbind("foo", "bar") => "foo=bar"
 */
char *
mkbind(name, value)
    char *name, *value;
{
    char *buf = xalloc(NULL, strlen(name) + 1 + strlen(value) + 1);

    sprintf(buf, "%s=%s", name, value);

    return buf;
}

/*
 *	Keyword processing (aka conditional).
 */

static char *keywords[MAXKEYWORDS] = {
    /* these will always match */
    "*",
    "all",

    /* operating systems */
#ifdef __linux__
    "linux",
#endif
#ifdef __APPLE__
    "apple",
#endif	
#ifdef DARWIN
    "darwin",
#endif
#ifdef __MACH__
    "mach",
#endif
#ifdef __unix__
    "unix"
#endif

	/* architectures */
#ifdef __i386__
    "i386",
#endif
#ifdef __i486__
    "i486",
#endif
#ifdef __i586__
    "i586",
#endif
#ifdef __i686__
    "i686",
#endif
#if defined(__i386__) || defined(__i486__) || defined(__i586__) || defined (__i686__)
    "ixxx",
    "intel",
#endif
#if defined(__ppc__) || defined(__POWERPC__) || defined(_ARCH_PPC)
    "ppc",
    "powerpc",
#endif
    NULL
};

/*
 *	Add a new word to the list of known keywords.
 */
void add_keyword(const char *word)
{
    char **kk;

    /* Don't add NULL words */
    if (word == NULL)
	return;

    /* Check if we might already got it */
    for (kk = keywords; *kk != NULL; kk++)
	if (strcasecmp(*kk, word) == 0)
	    return;

    *kk = xalloc(NULL, strlen(word) + 1);
    strcpy(*kk, word);
    *++kk = NULL;
}

/*
 *	Add qualified & unqualified hostname + all parent domains too.
 *
 *	Warning: Will trash hostname in the process.
 */
void add_keyword_hostname(char *hostname)
{
    char *p, *q;

    add_keyword(hostname);

    p = strchr(hostname, '.');
    if (p != NULL) {
	*p++ = '\0';
	add_keyword(hostname);
	while ((q = strchr(p, '.')) != NULL) {
	    add_keyword(p);
	    *q++ = '\0';
	    p = q;
	}
    }
}

/*
 *	Fill up the keywords array with more words that apply to us.
 */
void init_keywords(void)
{
    struct utsname uts;
    struct passwd *pw = getpwuid(getuid());
    char hostbuf[1024];

    add_keyword(getlogin());

    if (pw != NULL)
	add_keyword(pw->pw_name);

    if (gethostname(hostbuf, sizeof(hostbuf)) == 0)
	add_keyword_hostname(hostbuf);

    if (uname(&uts) == 0) {
	/* [<os>], e.g. [Linux] or [Darwin] */
	add_keyword(uts.sysname);

	/* [<nodename>], e.g. [lenux.lan.lovstrand.com] or [neo] */
	add_keyword_hostname(uts.nodename);

	/* [<machine>], e.g. [i686] or [Power Macintosh] */
	add_keyword(uts.machine);
    }
}

void list_keywords(void)
{
    char **kk;

    for (kk = keywords; *kk != NULL; kk++) {
	printf("%s\n", *kk);
    }
}

/*
 *	Determine if a certain "[name]" conditional applies to us.
 */
int conditional(const char *name)
{
    char **kk;

    /* try all predefined keywords */
    for (kk = keywords; *kk != NULL; kk++) {
	if (strcasecmp(name, *kk) == 0)
	    return TRUE;
    }

    /* if all else fails... well, we fail too */
    return FALSE;
}

int
auto_prune_paths()
{
    return AutoPrunePaths || (getenv("ESH_AUTO_PRUNE_PATHS") != NULL);
}

/*
 *	Read a binding from the stream and return it in the form "name=value".
 *	The format of the file is: name<whitespace>value<newline> with
 *	special processing for #, $, \.
 *
 *	Example:
 *		" name [=] value\"
 *		"	\#value\$value # comment"
 *		=> "name=value#value$value"
 */
char *
readbinding(stream)
    FILE *stream;
{
    char *p, *b, buf[BIGBUFSIZ];
    char *name, *value;
    int pathp = FALSE;
    int ignore = FALSE;

    name = value = NULL;
    b = buf;
    while (fgets(b, buf + sizeof(buf) - b, stream) != NULL) {

	/* find newline and nuke it */
	p = index(b, '\n');
	if (p == NULL)
	    fprintf(stderr,
		    "Warning: Line too long -- truncated after %d chars\n",
		    (int) sizeof(buf)-1);
	else
	    *p = '\0';

	/* find unquoted sharp (#) and nuke rest of line */
	for (p = b; *p != '\0'; p++) {
	    if (*p == '\\' && p[1] == '#')
		p++;
	    else if (*p == '#') {
		*p = '\0';
		break;
	    }
	}

	p = b;
	while (*p != '\0' && isspace(*p))
	    p++;
	if (*p == '\0')
	    continue;

	/* Is it a conditional "[name]" section? */
	if (*p == '[') {
	    char *n, name[1024];
	    int inexec = FALSE;
	    int intest = FALSE;

	    /* Assume that we will ignore this section */
	    ignore = TRUE;

	    for (n = name, p++; *p != '\0'; p++) {
		if (!inexec && !intest && (isspace(*p) || *p == ']')) {
		    *n = '\0';
		    if (n > name && conditional(name)) {
			ignore = FALSE;
			break;
		    }
		    n = name;
		} else if (*p == '`') {
		    if (inexec) {
			*n = '\0';
			if (system(name) == 0) {
			    ignore = FALSE;
			    break;
			}
			n = name;
		    }
		    inexec = !inexec;
		} else if (!intest && *p == '[') {
		    intest = TRUE;
		    *n++ = '[';
		    *n++ = ' ';
		} else if (intest && *p == ']') {
		    if (n < &name[sizeof(name)-2]) {
			*n++ = ' ';
			*n++ = ']';
		    }
		    *n = '\0';
		    if (system(name) == 0) {
			ignore = FALSE;
			break;
		    }
		    n = name;
		    intest = FALSE;

		} else if (n < &name[sizeof(name)-1]) {
		    *n++ = *p;
		}

		if (*p == ']')
		    break;
	    }
	    continue;
	}

	if (ignore)
	    continue;

	/* got a name already? */
	if (name == NULL) {
	    /* find beginning of name */
	    name = p;

	    /* find end of name */
	    while (*p != '\0' && !isspace(*p) && *p != '=')
		p++;
	    if (*p == '\0')
		return mkbind(name, "");
	    *p++ = '\0';

	    /* is this a path variable? */
	    if (auto_prune_paths() &&
		p - 5 >= b && strncmp(p - 5, "PATH", 4) == 0)
		pathp = TRUE;
	    b = p;
	}

	/* find beginning of value */
	while (*p != '\0' && isspace(*p))
	    p++;

	/* compact buffer if necessary */
	if (p > b) {
	    strcpy(b, p);
	    p = b;
	}

	if (value == NULL)
	    value = p;

	/* empty/end of value? */
	if (*p == '\0')
	    break;

	/* find end of value */
	p += strlen(p);
	if (p > value && p[-1] == '\\') {
	    /* handle continuation */
	    *--p = '\0';
	    b = p;
	    continue;
	} else {
	    /* remove trailing blanks */
	    while (p > value && isspace(p[-1]))
		p--;
	    if (p > value && p[-1] == '\\' && isspace(*p))
		p++;
	    *p = '\0';
	    break;
	}
    }
    if (name == NULL)
	return NULL;
    if (pathp) {
	extern char *ppath();
	char *bind;

	value = ppath(interpret(value, TRUE));
	bind = mkbind(name, value);
	free(value);
	return bind;
    } else
	return mkbind(name, interpret(value, FALSE));
}

/*
 *	Interpret the given string with respect to variables etc.
 *	(Result is static string)
 */
char *
interpret(string, pathcompress)
    char *string;
    int pathcompress;
{
    static char tmp[BIGBUFSIZ];
    static char buf[BIGBUFSIZ];
    char *p, *q;

    if (string == NULL)
	return NULL;

    p = strcpy(tmp, string);
    q = buf;

    while (*p != '\0' && q < &buf[sizeof(buf)] - 1) {
	char *oq = q;

	switch (*p++) {
	  case '$':
	    expand(&p, &q, &buf[sizeof(buf)] - q);
	    if (pathcompress && q == oq) {
		/* Don't let an empty expansion lead to an empty
		 * path component being created.
		 */
		if (q > buf && q[-1] == ':' && *p == '\0')
		    q--;
		else if ((q == buf || q[-1] == ':') && *p == ':')
		    p++;
	    }
	    break;
	  case '`':
	    compute(&p, &q, &buf[sizeof(buf)] - q);
	    if (pathcompress && q == oq) {
		/* Don't let an empty expansion lead to an empty
		 * path component being created.
		 */
		if (q > buf && q[-1] == ':' && *p == '\0')
		    q--;
		else if ((q == buf || q[-1] == ':') && *p == ':')
		    p++;
	    }
	    break;
	  case '\\':
	    switch (*p) {
	      case 'n': *q++ = '\n'; break;
	      case 'r': *q++ = '\r'; break;
	      case 't': *q++ = '\t'; break;
	      default:
		if (isdigit(*p))
		    *q++ = strtol(p, &p, 8);
		else
		    *q++ = *p++;
	    }
	    break;
	  default:
	    *q++ = *(p-1);
	    break;
	}
    }
    *q = '\0';

    return buf;
}

/*
 *	Parse and expand the given variable that src points to and
 *	copy the result into dst.  Update both src and dst pointers.
 *	Eg. expand("$foo/baz", "xxxxxxxx", 8) with environ = {"foo=bar", NULL}
 *	would give "$foo/baz", "barxxxxx"
 *	      with      ^=src      ^=dst
 */
void
expand(src, dst, dstlen)
    char **src, **dst;
    int dstlen;
{
    register char *p, *q;
    char delim, *value, *defvalue = "";
    int brace = FALSE;

    if (**src == '{' || **src == '(') {
	(*src)++;
	brace = TRUE;
    }

    p = *src;
    while (*p != '\0' && (isalnum(*p) || *p == '_')) p++;

    delim = *p;
    *p = '\0';
    value = getenv(*src);
    if (brace) {
	*p = delim;
	if (delim != '\0' && delim != '}' && delim != ')')
	    defvalue = ++p;
	while (*p != '\0' && *p != '}' && *p != ')') p++;
	delim = *p;
	*p = '\0';
    }
    *src = p;

    if (value == NULL)
	value = defvalue;

    /* copy value to dst */
    for (p = value, q = *dst; *p != '\0' && dstlen > 0; p++, q++, dstlen--)
	*q = *p;

    /* put back source delimiter update pointers */
    **src = delim;
    if (brace && (delim == '}' || delim == ')'))
	(*src)++;
    *dst = q;
}

/*
 *	Parse and compute the given command by running it through a pipe and
 *	picking up the result.  Will update both src and dst pointers.
 *	Eg. compute("`arch`/foo", "xxxxxxxx", 8)
 *	would give  "`arch`/foo", "sun4xxxx"
 *	      with         ^=src       ^=dst
 */
void
compute(src, dst, dstlen)
    char **src, **dst;
    int dstlen;
{
    char *p, *q, delim;
    FILE *pipe;

    p = index(*src, '`');
    if (p == NULL)
	p = *src + strlen(*src);
    delim = *p;
    *p = '\0';

    pipe = popen(*src, "r");
    if (pipe == NULL) {
	perror(*src);
    } else {
	if (fgets(*dst, dstlen, pipe) == NULL)
	    **dst = '\0';
	if ((q = index(*dst, '\n')) != NULL)
	    *q = '\0';

	pclose(pipe);
    }

    *p = delim;
    if (delim != '\0')
	p++;

    *src = p;
    *dst += strlen(*dst);
}

/*
 *	A version of assoc(iate) for foo=bar type bindings.
 *	Eg. bassoc("foo", {"abc=def", "foo=bar", NULL}) => {"foo=bar", ...}
 *	    bassoc("foo=xyz", {"abc=def", "foo=bar", NULL}) => {"foo=bar", ...}
 *	    bassoc("baz", {"abc=def", "foo=bar", NULL}) => {NULL}
 */
char **
bassoc(key, environ)
    const char *key;
    char **environ;
{
    register char **ee;
    register const char *p, *q;

    for (ee = environ; *ee != NULL; ee++) {
	for (p = key, q = *ee; *p == *q; p++, q++)
	    if (*p == '\0' || *p == '=')
		break;
	if ((*p == '\0' || *p == '=') && (*q == '\0' || *q == '='))
	    break;
    }

    return ee;
}

/*
 * Edit our environment by defaulting, replacing, removing, or appending
 * the given binding (which should be of the form "var=val").
 */
void
editenv(enum editop op, const char *binding)
{
    static char **EnvEnd = NULL;
    int envuse = EnvEnd - EnvBuf;
    
    if (op != OP_APPEND) {
	char **ee = bassoc(binding, EnvBuf);
	if (*ee != NULL) {
	    switch (op) {
		/* Only add the binding if the var is unbound */
	      case OP_DEFAULT:
		return;
	      case OP_REPLACE:
		/* Replace old binding */
		*ee = (char *) binding;
		return;
	      case OP_REMOVE:
		/* Remove existing binding */
		for (; *ee != NULL; ee++) {
		    ee[0] = ee[1];
		}
		EnvEnd--;
		return;
	      case OP_APPEND:
		/* To keep the compiler happy... */
		break;
	    }
	}
    }

    /* Append the new binding */
    if (envuse == EnvSiz) {
	EnvSiz += ENVGROWTH;
	environ = EnvBuf = xalloc(EnvBuf, sizeof(char **) * (EnvSiz + 1));
	EnvEnd = EnvBuf + envuse;
    }
    
    *EnvEnd++ = (char *) binding;
    *EnvEnd = NULL;
}

/*
 *	Print and automatically quote a string (sh/csh syntax).
 */
void
fprintq(stream, string)
    FILE *stream;
    char *string;
{
    char *p;

    if (ShellOut == LISP_FORMAT) {
	putc('"', stream);
	for (p = string; *p != '\0'; p++) {
	    if (*p == '"' || *p == '\\')
		putc('\\', stream);
	    putc(*p, stream);
	}
	putc('"', stream);

    } else {
	putc('\'', stream);
	for (p = string; *p != '\0'; p++)
	    if (*p == '\'')
		fputs("'\"'\"'", stream);
	    else
		putc(*p, stream);
	putc('\'', stream);
    }
}

/*
 *	Copy the specified string into a newly malloced space.
 */
char *
newstr(string)
    char *string;
{
    char *result = xalloc(NULL, strlen(string) + 1);

    (void) strcpy(result, string);

    return result;
}

/*
 *	Attempt to (re)allocate siz bytes and exit on failure.
 */
void *
xalloc(void *mem, long siz)
{
    mem = (mem == NULL) ? malloc(siz) : realloc(mem, siz);

    if (mem == NULL) {
	perror("malloc");
	exit(1);
    }

    return mem;
}
