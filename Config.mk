# $Id$

CC = musl-gcc -D__musl__
CFLAGS = -std=c99 -g -DEXTENDED -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600 -DFILEC -DNLS -DSHORT_STRINGS -D_KMEMUSER -Dunix -D_NETBSD_SOURCE -D_BSD_SOURCE=1 -D__BSD_VISIBLE=1 -D__P"(x)"=x -fcommon -I. -Werror -I$(TOPDIR)
LDFLAGS =
LIBS =
