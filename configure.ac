# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.59])
AC_INIT([ktf], [1.0], [knut.omang@oracle.com])

# unique source file --- primitive safety check
AC_CONFIG_SRCDIR([kernel/ktf.h])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([ac])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Look for/generate m4 files under top/m4
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Silent rules by default - use make V=1 for verbose
AM_SILENT_RULES([yes])


#  temporary measure to see if running make sequentially will eliminate some
#  spurious build errors:
#  NB! this comment is interpreted by terse, keep as is:
# Sequential make

# Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O

AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

AC_ARG_WITH([googletest], AS_HELP_STRING([--with-googletest], [Optional path to a gtest install]),
	[gtest_prefix="$withval"],AS_IF([test "x$prefix" != "xNONE" -a "x$prefix" != "x"],[gtest_prefix=$prefix]))

AM_CONFIG_KTF
AM_KTF_DIR([kernel])
AM_KTF_DIR([selftest])
AM_KTF_DIR([examples])

AC_CONFIG_FILES([Makefile
		 kernel/Makefile
		 selftest/Makefile
		 examples/Makefile
		 lib/Makefile
		 user/Makefile
		 doc/Makefile
])
AC_OUTPUT
