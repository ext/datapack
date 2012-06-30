AM_CFLAGS = -I ${top_srcdir}

noinst_PROGRAMS = sample
sample_LDADD = ../libdatapack.la
sample_SOURCES = files.c main.c

CLEANFILES = files.c files.h

files.c: datafiles Makefile
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -d $(DEPDIR)/$(notdir $<).d -o $@ -e $(basename $@).h

-include ./$(DEPDIR)/datafiles.d
