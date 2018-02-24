#
#	Makefile for ppath and esh
#
#	Lennart Lovstrand, Rank Xerox EuroPARC, January 1990.
#

PREFIX=	/usr/local
ETCDIR=	$(PREFIX)/etc
BINDIR=	$(PREFIX)/bin
MANDIR=	$(PREFIX)/share/man/man1
SHELLS=	/etc/shells

INSTALL=install -c -s
COPY=	cp -p
MKDIR=	mkdir -p
SED=	sed

CFLAGS=	-O -g -Wall -DAUTO_PRUNE_PATH -DDISABLE_NONINTERACIVE_PS1 \
	-DETCDIR=\"$(ETCDIR)\" $(EXTRACFLAGS)

all:	ppath esh esh.1

ppath:	ppath.o ppathmain.o
	$(CC) -g $(EXTRACFLAGS) -o ppath ppath.o ppathmain.o

esh:	esh.o ppath.o
	$(CC) -g $(EXTRACFLAGS)  -o esh esh.o ppath.o

esh.1:	esh.1.sed
	$(SED) -e "s:ETCDIR:$(ETCDIR):g" esh.1.sed >esh.1

install:	all $(BINDIR) $(ETCDIR) $(MANDIR)
	$(INSTALL) ppath esh $(BINDIR)
	$(COPY) esh.1 $(MANDIR)
	test -f "$(ETCDIR)/environ" || $(COPY) environ $(ETCDIR)
	@if [ -e "$(SHELLS)" ] && ! fgrep -q /esh "$(SHELLS)"; then \
	    echo; \
	    echo "*** Note: You will need to manually add $(BINDIR)/esh to"; \
	    echo "*** $(SHELLS) if you want to be able to have it as a login shell."; \
	fi

$(BINDIR) $(ETCDIR) $(MANDIR):
	$(MKDIR) $@

clean:	;-rm esh ppath *.o esh.1
