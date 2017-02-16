# Process this file with autoconf to produce a configure script.

# Prelude.
AC_PREREQ([2.59])
AC_INIT([ktest], [1.0], [knut.omang@oracle.com])

# unique source file --- primitive safety check
AC_CONFIG_SRCDIR([kernel/ktest.h])

# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([ac])

# Look for/generate m4 files under top/m4
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Silent rules by default - use make V=1 for verbose
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AM_PROG_CC_C_O

AM_PROG_AR
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.1, [have_libnl3=yes],[ dnl
  have_libnl3=no
  PKG_CHECK_MODULES([NETLINK], [libnl-1 >= 1.1])
])

if (test "${have_libnl3}" = "yes"); then
        NETLINK_CFLAGS+=" $LIBNL3_CFLAGS"
        NETLINK_LIBS+=" $LIBNL3_LIBS -lnl-genl-3"
	AC_DEFINE([HAVE_LIBNL3], 1, [Using netlink v.3])
else
	LIBS+=" $NETLINK_LIBS"
fi


AS_IF([ test x$KVER != x ],dnl
   	[AC_MSG_NOTICE("building against kernel version $KVER")],dnl
    	[AC_MSG_ERROR("Kernel version (KVER) not set")])

AC_ARG_VAR([KVER],[Kernel version to use])

AM_CONFIG_KTEST
AM_KTEST_DIR([kernel])
AM_KTEST_DIR([selftest])

AC_CONFIG_FILES([Makefile
		 kernel/Makefile
		 selftest/Makefile
		 lib/Makefile
		 user/Makefile
])
AC_OUTPUT