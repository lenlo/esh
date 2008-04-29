#
#	Makefile for ppath and esh
#
#	Lennart Lovstrand, Rank Xerox EuroPARC, January 1990.
#

PREFIX=	/usr/local
ETCDIR=	$(PREFIX)/etc
BINDIR=	$(PREFIX)/bin
MANDIR=	$(PREFIX)/share/man/man1

INSTALL=install -c -s
COPY=	cp -p
MKDIR=	mkdir -p

CFLAGS=	-O -g -Wall -DAUTO_PRUNE_PATH -DREMOVE_EMPTY_PATHS \
	-DDISABLE_NONINTERACIVE_PS1 -DETCDIR=\"$(ETCDIR)\" \
	#-DHAVE_WRITABLE_STRINGS -fwritable-strings

all:	ppath esh

ppath:	ppath.o ppathmain.o
	$(CC) -g -o ppath ppath.o ppathmain.o

esh:	esh.o ppath.o
	$(CC) -g -o esh esh.o ppath.o

install:	all $(BINDIR) $(ETCDIR)
	$(INSTALL) ppath esh $(BINDIR)
	$(COPY) environ $(ETCDIR)
	$(COPY) esh.1 $(MANDIR)

$(BINDIR) $(ETCDIR):
	$(MKDIR) $@

clean:	;-rm esh ppath *.o
