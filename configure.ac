AC_INIT([datapack],[3],[ext@sidvind.com])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile sample/Makefile])

AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_LIBTOOL([disable-static])
AC_DEFINE_UNQUOTED([SRCDIR], ["${srcdir}/"], [srcdir])

AC_OUTPUT
