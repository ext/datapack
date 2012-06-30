AM_CFLAGS = -I ${top_srcdir}

DATAFILES = files.dpl
noinst_PROGRAMS = sample
sample_LDADD = ${top_builddir}/libdatapack.la
sample_SOURCES = $(DATAFILES) main.c

BUILT_SOURCES = $(DATAFILES:.dpl=.c)
CLEANFILES = $(DATAFILES:.dpl=.c) $(DATAFILES:.dpl=.h)

.dpl.c:
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -d $(DEPDIR)/$(notdir $<).d -e $(basename $@).h -o $@

-include $(addprefix ./$(DEPDIR)/, $(basename $(DATAFILES:.dpl=.dpl.d)))
