ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -Wall -Wvla -Wconversion -I ${top_srcdir}

SUBDIRS = . sample

bin_PROGRAMS = datapacker
lib_LTLIBRARIES = libdatapack.la

datapacker_LDADD = -lz
datapacker_SOURCES = pack.c datapack.h pak.h

libdatapack_la_LIBADD = -lz -ldl
libdatapack_la_SOURCES = unpack.c datapack.h pak.h

include_HEADERS = datapack.h

EXTRA_DIST = \
	sample/data1.txt \
	sample/data2.txt \
	sample/data3.txt \
	tests/data1.dpl \
	tests/data1.txt \
	tests/data2.dpl

# pkg-config
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = datapack.pc

# Unit-testing
TESTS = tests/test
check_PROGRAMS = $(TESTS)
tests_test_CXXFLAGS = -Itests
tests_test_LDFLAGS = -rdynamic
tests_test_LDADD = libdatapack.la -lcppunit
tests_test_SOURCES = tests/test.cpp
nodist_tests_test_SOURCES = tests/data1.c
tests/test.cpp: tests/data1.c tests/data2.pak

CLEANFILES = tests/data1.c tests/data1.h tests/data2.pak

.dpl.c: datapacker Makefile
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -e $(basename $@).h -o $@

.dpl.pak: datapacker Makefile
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -t bin -o $@
