m4_define([_VERSION_MAJOR], [0])
m4_define([_VERSION_MINOR], [3])
m4_define([_VERSION_MICRO], [1])
m4_define([_VERSION_NUM], m4_join([.],_VERSION_MAJOR, _VERSION_MINOR, _VERSION_MICRO))

AC_INIT([datapack],_VERSION_NUM,[ext@sidvind.com])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile sample/Makefile datapack.pc deb-control])

AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_CXX
AC_PROG_LIBTOOL([disable-static])
AC_DEFINE_UNQUOTED([SRCDIR], ["${srcdir}/"], [srcdir])
AC_CHECK_HEADERS([getopt.h libgen.h dirent.h endian.h])

VERSION_MAJOR=_VERSION_MAJOR
VERSION_MINOR=_VERSION_MINOR
VERSION_MICRO=_VERSION_MICRO
AC_DEFINE_UNQUOTED([VERSION_MAJOR],  [${VERSION_MAJOR}], [Major version number])
AC_DEFINE_UNQUOTED([VERSION_MINOR],  [${VERSION_MINOR}], [Minor version number])
AC_DEFINE_UNQUOTED([VERSION_MICRO],  [${VERSION_MICRO}], [MICRO version number])

ARCH=`echo $host|cut -d '-' -f 1`
AS_IF([test "$ARCH" = "i686"], [ARCH="i386"])
AS_IF([test "$ARCH" = "x86_64"], [ARCH="amd64"])
AC_SUBST(ARCH)

AC_OUTPUT
