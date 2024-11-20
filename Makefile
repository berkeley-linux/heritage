# $Id$

.PHONY: all build-usr-bin build-usr-sbin clean makefiles distclean

all: build-usr-bin build-usr-sbin

makefiles: Makefile.in
	cd usr.bin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done
	cd usr.sbin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done

build-usr-bin: makefiles
	cd usr.bin && for i in *; do \
		$(MAKE) -C $$i || break ; \
	done

build-usr-sbin: makefiles
	cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i || break ; \
	done

clean: makefiles
	cd usr.bin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done
	cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done

distclean: makefiles clean
	rm -f */*/Makefile
