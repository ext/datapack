ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -I ${top_srcdir}

bin_PROGRAMS = datapacker
noinst_PROGRAMS = sample
lib_LTLIBRARIES = libdatapack.la

datapacker_LDADD = -lz
datapacker_SOURCES = pack.c datapack.h

libdatapack_la_LIBADD = -lz
libdatapack_la_SOURCES = unpack.c datapack.h

sample_LDADD = libdatapack.la
sample_SOURCES = files.c main.c

CLEANFILES = files.c

files.c: datapacker ${top_srcdir}/data1.txt ${top_srcdir}/data2.txt ${top_srcdir}/data3.txt Makefile
	$(AM_V_GEN)./datapacker DATA1:${top_srcdir}/data1.txt DATA2:${top_srcdir}/data2.txt DATA3:${top_srcdir}/data3.txt -o files.c -e files.h
