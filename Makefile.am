ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wvla -Wconversion -I ${top_srcdir}

SUBDIRS = . sample

bin_PROGRAMS = datapacker
lib_LTLIBRARIES = libdatapack.la

datapacker_LDADD = -lz
datapacker_SOURCES = pack.c datapack.h

libdatapack_la_LIBADD = -lz -ldl
libdatapack_la_SOURCES = unpack.c datapack.h

include_HEADERS = datapack.h

EXTRA_DIST = \
	sample/data1.txt \
	sample/data2.txt \
	sample/data3.txt

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = datapack.pc

# Unit-testing
DATAFILES = tests/data1.dpl
TESTS = tests/test
check_PROGRAMS = $(TESTS)
tests_test_CXXFLAGS = -Itests
tests_test_LDADD = libdatapack.la -lcppunit
tests_test_SOURCES = tests/test.cpp $(DATAFILES)
tests/test.c: $(DATAFILES:.dpl=.c)

CLEANFILES = $(DATAFILES:.dpl=.c) $(DATAFILES:.dpl=.h)

.dpl.c: datapacker Makefile
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -d $(DEPDIR)/$(notdir $<).d -e $(basename $@).h -o $@

-include $(addprefix ./$(DEPDIR)/, $(basename $(DATAFILES:.dpl=.dpl.d)))
