/**
 **	PPATH -- Prune Path
 **
 **	Remove all duplicate directories from a path string.
 **
 **	Usage: ppath("foo:bar:baz:foo") => foo:bar:baz
 **
 **	Lennart Lovstrand, Rank Xerox EuroPARC, England.
 **	Created: Thu Jan 11 10:54:23 1990
 **	Last edited: Thu May 27 17:18:06 2004
 **/

#define	NULL		0
#define TRUE		1
#define FALSE		0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ppath_remove_empty_subpaths = -1;

static char *xalloc(int);
char **breakup(char *), **remove_duplicates(char **);

char *
ppath(path)
char *path;
{
    int remove_empty_subpaths = ppath_remove_empty_subpaths;

    char *result, **strings, **pp;
    int length = 0;
    int colon;

    if (remove_empty_subpaths == -1) {
	remove_empty_subpaths =
	    (getenv("PPATH_REMOVE_EMPTY_SUBPATHS") != NULL);
    }

    strings = remove_duplicates(breakup(path));

    for (pp = strings; *pp != NULL; pp++)
	length += strlen(*pp) + 1;

    result = (char *) xalloc(length);
    *result = '\0';

    colon = FALSE;
    for (pp = strings; *pp != NULL; pp++) {
	if (remove_empty_subpaths && **pp == '\0')
	    continue;
	if (colon)
	    (void) strcat(result, ":");
	else
	    colon = TRUE;
	(void) strcat(result, *pp);
	(void) free(*pp);
    }

    return result;
}

char **
breakup(path)
    char *path;
{
    int parts, i;
    char *p, *q, **result;

    parts = 1;
    p = path - 1;
    while ((p = index(p + 1, ':')) != NULL)
	parts++;

    result = (char **) malloc((parts + 1) * sizeof(char *));

    p = path;
    for (i = 0; i < parts; i++) {
	q = index(p, ':');
	if (q == NULL)
	    q = &p[strlen(p)];
	result[i] = xalloc(q - p + 1);
	(void) strncpy(result[i], p, q - p);
	result[i][q - p] = '\0';
	p = q + 1;
    }
    result[i] = NULL;
    
    return result;
}

char **
remove_duplicates(strings)
    char **strings;
{
    int i, j, k;

    if (strings[0] == NULL)
	return strings;

    for (i = 0; strings[i] != NULL; i++)
	for (j = i + 1; strings[j] != NULL; j++)
	    while (strings[j] != NULL && strcmp(strings[i], strings[j]) == 0) {
		free(strings[j]);
		for (k = j + 1; strings[k] != NULL; k++)
		    strings[k - 1] = strings[k];
		strings[k - 1] = NULL;
	    }

    return strings;
}

static char *
xalloc(bytes)
    int bytes;
{
    register char *result;

    result = (char *) malloc(bytes);
    if (result == NULL) {
	perror("malloc");
	exit(1);
    }

    return result;
}
