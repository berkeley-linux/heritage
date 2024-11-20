# $Id$

.PHONY: all build clean makefiles distclean

all: build

makefiles: Makefile.in
	for i in *; do \
		if [ -d "$$i" ]; then \
			sed "s/@output@/$$i/g" Makefile.in | sed "s/@objects@/`cd $$i && ls -d *.c | tr '\n' ' ' | sed -E 's/\.c/.o/g'`/g" > $$i/Makefile ; \
		fi \
	done

build: makefiles
	for i in *; do \
		if [ -d "$$i" ]; then \
			$(MAKE) -C $$i || break ; \
		fi \
	done

clean: makefiles
	for i in *; do \
		if [ -d "$$i" ]; then \
			$(MAKE) -C $$i clean ; \
		fi \
	done

distclean: makefiles clean
	rm -f */Makefile
