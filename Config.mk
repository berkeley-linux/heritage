# $Id$

PREFIX = /usr/heritage

CC = gcc
CFLAGS = -std=c99 -g -DEXTENDED -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DFILEC -DNLS -DSHORT_STRINGS -D_KMEMUSER -Dunix -D_NETBSD_SOURCE -D_BSD_SOURCE=1 -D__BSD_VISIBLE=1 -DNET2_STAT -DPREFIX=\"$(PREFIX)\" -fcommon -I. -Werror -I$(TOPDIR)
LDFLAGS =
LIBS =

# include $(TOPDIR)/makefiles/musl-host.mk
# include $(TOPDIR)/makefiles/musl-gcc.mk
