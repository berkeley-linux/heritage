# $Id$

.PHONY: all build-bin build-sbin clean makefiles distclean

all: build-bin build-sbin

makefiles: Makefile.in
	cd bin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done
	cd sbin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done

build-bin: makefiles
	cd bin && for i in *; do \
		$(MAKE) -C $$i || break ; \
	done

build-sbin: makefiles
	cd sbin && for i in *; do \
		$(MAKE) -C $$i || break ; \
	done

clean: makefiles
	cd bin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done
	cd sbin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done

distclean: makefiles clean
	rm -f */*/Makefile
