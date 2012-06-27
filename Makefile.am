ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wvla -Wconversion -I ${top_srcdir}

bin_PROGRAMS = datapacker
lib_LTLIBRARIES = libdatapack.la

datapacker_LDADD = -lz
datapacker_SOURCES = pack.c datapack.h

libdatapack_la_LIBADD = -lz
libdatapack_la_SOURCES = unpack.c datapack.h

# Usage sample
noinst_PROGRAMS = sample
sample_LDADD = libdatapack.la
sample_SOURCES = files.c sample/main.c

CLEANFILES = files.c

files.c: ${top_srcdir}/sample/datafiles datapacker Makefile
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s ${top_srcdir}/sample -d $(DEPDIR)/files.data -o $@ -e files.h

-include ./$(DEPDIR)/files.data
