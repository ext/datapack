AM_CFLAGS = -I ${top_srcdir}

noinst_PROGRAMS = method1 method2 method3
DATAFILES = files.dpl

# Files packed into binary and read using handle
method1_LDADD = ${top_builddir}/libdatapack.la
method1_SOURCES = files.dpl method1.c

# Files packed into binary and read using virtual filename
method2_LDADD = ${top_builddir}/libdatapack.la
method2_LDFLAGS = ${AM_LDFLAGS} -rdynamic
method2_SOURCES = files.dpl method2.c

# Files packed into blob and read using virtual filename
method3_LDADD = ${top_builddir}/libdatapack.la
method3_SOURCES = files.pak method3.c

BUILT_SOURCES = $(DATAFILES:.dpl=.c) $(DATAFILES:.dpl=.pak)
CLEANFILES = $(DATAFILES:.dpl=.c) $(DATAFILES:.dpl=.h) $(DATAFILES:.dpl=.pak)

.dpl.c:
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -d $(DEPDIR)/$(notdir $<).d -e $(basename $@).h -o $@

.dpl.pak:
	@test x"$(@D)" = x || $(MKDIR_P) "$(@D)"
	$(AM_V_GEN)${top_builddir}/datapacker -f $< -s $(dir $<) -t bin -o $@

-include $(addprefix $(DEPDIR)/, $(DATAFILES:=.d))
