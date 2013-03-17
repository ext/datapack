ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wvla -Wconversion -I ${top_srcdir}

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

files.c: datapacker ${top_srcdir}/datafiles Makefile
	$(AM_V_GEN)./datapacker -f ${top_srcdir}/datafiles -s ${top_srcdir} -d $(DEPDIR)/files.data -o files.c -e files.h

-include ./$(DEPDIR)/files.data
