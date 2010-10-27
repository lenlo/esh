/**
 **	PPATH -- Prune Path
 **
 **	Remove all duplicate directories from a path string.
 **
 **	Usage: ppath foo:bar:baz:foo => foo:bar:baz
 **	as in: PATH=`ppath $PATH`
 **
 **	Lennart Lovstrand, Rank Xerox EuroPARC, England.
 **	Created: Thu Jan 11 10:54:23 1990
 **     Last edited: Thu May 27 17:19:14 2004
 **/

#include <stdio.h>
#include <stdlib.h>

#define TRUE		1
#define FALSE		0

extern char *ppath();
extern int ppath_remove_empty_subpaths;

void usage(pname)
char *pname;
{
    fprintf(stderr, "usage: %s [-z] [-e] path\n", pname);
    exit(1);
}

int
main(argc, argv)
int argc;
char **argv;
{
    int ac;
    int next_is_path = FALSE;

    for (ac = 1; ac < argc && argv[ac][0] == '-' && !next_is_path; ac++) {
	const char *opt;
	for (opt = &argv[ac][1]; *opt != '\0'; opt++) {
	    switch (*opt) {
	      case 'e':
		next_is_path = TRUE;
		break;
	      case 'z':
		ppath_remove_empty_subpaths = TRUE;
		break;
	      default:
		usage(argv[0]);
		break;
	    }
	}
    }

    if (argc - ac != 1)
	usage(argv[0]);

    printf("%s\n", ppath(argv[ac]));

    return 0;
}
