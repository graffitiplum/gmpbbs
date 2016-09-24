CC=gcc
LIBTOOL=libtool

#COPT=-O3 -march=prescott -mmmx -msse -msse2 -msse3 -DHWRANDOM=\"/dev/urandom\"
COPT=-O3 -march=nocona -mmmx -msse -msse2 -msse3 -DHWRANDOM=\"/dev/urandom\"

CFLAGS=-Wall $(COPT) -pipe -D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS=
LIBS=-lm -lgmp

PREFIX=/usr
BINDIR=$(PREFIX)/bin
LIBDIR=$(PREFIX)/lib
INCLUDEDIR=$(PREFIX)/include
SHAREDIR=$(PREFIX)/share
MANDIR=$(SHAREDIR)/man
INFODIR=$(SHAREDIR)/info

SHARED=libgmpbbs.so
LIBOBJ=gmpbbs.lo
OBJ=main.lo

all: $(SHARED) gmpbbs

gmpbbs: $(OBJ)
	$(LIBTOOL) --mode=link $(CC) $(CFLAGS) -o gmpbbs \
		$(OBJ) $(LDFLAGS) libgmpbbs.la $(LIBS)

%.lo:
	$(LIBTOOL) --mode=compile $(CC) $(CFLAGS) -c $(subst lo,c,$@)

$(SHARED): $(LIBOBJ)
	$(LIBTOOL) --mode=link $(CC) -shared $(CFLAGS) -o libgmpbbs.la \
		$(LIBOBJ) -rpath $(LIBDIR)

install: $(SHARED) gmpbbs
	install -d $(BINDIR)
	install -d $(LIBDIR)
	install -d $(INCLUDEDIR)
	$(LIBTOOL) --mode=install install -c gmpbbs $(BINDIR)/gmpbbs
	install -m 0644 gmpbbs.h $(INCLUDEDIR)/gmpbbs.h
	$(LIBTOOL) --mode=install install -c libgmpbbs.la $(LIBDIR)

clean:
	rm -rf *~ *.o *.lo .libs gmpbbs libgmpbbs.la
