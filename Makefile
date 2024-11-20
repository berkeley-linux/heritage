# $Id$

.PHONY: all build-usr-bin build-usr-sbin build-bin clean makefiles distclean

all: build-usr-bin build-usr-sbin build-bin

makefiles: Makefile.in
	cd usr.bin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done
	cd usr.sbin && for i in *; do \
		sed "s/@output@/$$i/g" ../Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
	done
	cd bin && for i in *; do \
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

bin/csh/const.h: bin/csh/const.c
	rm -f $@
	$(CC) -E $(CFLAGS) bin/csh/const.c | egrep 'Char STR' | sed -e 's/Char \([a-zA-Z0-9_]*\)\(.*\)/extern Char \1[];/' | sort >> $@

bin/csh/err.h: bin/csh/err.c
	rm -f $@
	echo '#ifndef _h_sh_err' >> $@
	echo '#define _h_sh_err' >> $@
	egrep 'ERR_' bin/csh/err.c | egrep '^#define' >> $@
	echo '#endif' >> $@

build-bin: makefiles bin/csh/err.h bin/csh/const.h
	cd bin && for i in *; do \
		$(MAKE) -C $$i || break ; \
	done

clean: makefiles
	cd usr.bin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done
	cd usr.sbin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done
	cd bin && for i in *; do \
		$(MAKE) -C $$i clean ; \
	done

distclean: makefiles clean
	rm -f */*/Makefile
