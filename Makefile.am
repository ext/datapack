ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wvla -Wconversion -I ${top_srcdir}

bin_PROGRAMS = datapacker
lib_LTLIBRARIES = libdatapack.la

datapacker_LDADD = -lz
datapacker_SOURCES = pack.c datapack.h

libdatapack_la_LIBADD = -lz
libdatapack_la_SOURCES = unpack.c datapack.h

SUBDIRS = . sample
