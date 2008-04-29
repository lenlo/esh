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

void usage(pname)
char *pname;
{
    fprintf(stderr, "usage: %s path\n", pname);
    exit(1);
}

int
main(argc, argv)
int argc;
char **argv;
{
    if (argc != 2)
	usage(argv[0]);

    printf("%s\n", ppath(argv[1]));

    return 0;
}
