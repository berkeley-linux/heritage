# $Id$

TOPDIR = .
include Config.mk

.PHONY: all build-usr-bin build-usr-sbin build-bin clean install makefiles distclean

all: build-usr-bin build-usr-sbin build-bin

makefiles: Makefile.in
	cd usr.bin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" | sed "s/@ifreq@//g" > $$i/Makefile ; \
	done
	cd usr.sbin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" | sed "s/@ifreq@//g" > $$i/Makefile ; \
	done
	cd bin && for i in *; do \
		if [ "$$i" = "date" ]; then \
			IFREQ="-lutil" ; \
		else \
			IFREQ="" ; \
		fi ; \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" | sed "s/@ifreq@/$$IFREQ/g" > $$i/Makefile ; \
	done

build-usr-bin: makefiles
	TOPDIR="`pwd`" && cd usr.bin && for i in *; do \
		$(MAKE) -C $$i TOPDIR="$$TOPDIR" || break ; \
	done

build-usr-sbin: makefiles
	TOPDIR="`pwd`" && cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i TOPDIR="$$TOPDIR" || break ; \
	done

build-bin: makefiles bin/csh/csherr.h bin/csh/const.h
	TOPDIR="`pwd`" && cd bin && for i in *; do \
		$(MAKE) -C $$i TOPDIR="$$TOPDIR" || break ; \
	done

clean: makefiles
	TOPDIR="`pwd`" && cd usr.bin && for i in *; do \
		$(MAKE) -C $$i clean TOPDIR="$$TOPDIR" ; \
	done
	TOPDIR="`pwd`" && cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i clean TOPDIR="$$TOPDIR" ; \
	done
	TOPDIR="`pwd`" && cd bin && for i in *; do \
		$(MAKE) -C $$i clean TOPDIR="$$TOPDIR" ; \
	done
	rm -f bin/csh/const.h bin/csh/csherr.h

install: all
	TOPDIR="`pwd`" && cd usr.bin && for i in *; do \
		$(MAKE) -C $$i install TOPDIR="$$TOPDIR" DESTDIR=$(DESTDIR) ; \
	done
	TOPDIR="`pwd`" && cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i install TOPDIR="$$TOPDIR" DESTDIR=$(DESTDIR) ; \
	done
	TOPDIR="`pwd`" && cd bin && for i in *; do \
		$(MAKE) -C $$i install TOPDIR="$$TOPDIR" DESTDIR=$(DESTDIR) ; \
	done

distclean: makefiles clean
	rm -f */*/Makefile

### csh ###

bin/csh/const.h: bin/csh/const.c
	rm -f $@
	$(CC) -E $(CFLAGS) bin/csh/const.c | egrep 'Char STR' | sed -e 's/Char \([a-zA-Z0-9_]*\)\(.*\)/extern Char \1[];/' | sort >> $@

bin/csh/csherr.h: bin/csh/err.c
	rm -f $@
	echo '#ifndef _h_sh_err' >> $@
	echo '#define _h_sh_err' >> $@
	egrep 'ERR_' bin/csh/err.c | egrep '^#define' >> $@
	echo '#endif' >> $@
