#
# Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
#    Author: Knut Omang <knut.omang@oracle.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
#
# Dependent modules should make this file available in their m4 path
# and call this from their configure.ac:
#
# Require ktf and its dependencies (setting up KTF):
#
# AM_LIB_KTF
#
# For each subdirectory where kernel test modules are located within the project, call:
# AM_KTF_DIR([subdirectory])
#
#

# Helper function to check if a pointer to a kernel to build against is set:
#
AC_DEFUN([AC_CHECK_KPATH],
[
dnl Fail on attempts to configure using a relative path to the source tree:
AS_IF([test "x${ac_confdir%%/*}" != "x" -a "x${ac_confdir%%/*}" != "x." ],dnl
	[AC_MSG_ERROR([$ac_confdir: Relative paths not supported
  - please invoke configure with an absolute path!])])

dnl We implicitly set KDIR from KVER if it is not set explicitly
AS_IF([test "x$KDIR" = "x" -a "x$KVER" != x],[KDIR='/lib/modules/$(KVER)/build'])

AS_IF([ test x$KVER != x -a "x$KDIR" = x ],dnl
	[AC_MSG_ERROR([dnl
 Neither environment KVER nor KDIR is set!
 - Set KDIR to an installed kernel version as output by "uname -r"
   Alternatively set KDIR to the directory of the toplevel Makefile of a kernel build environment])]
)

AC_ARG_VAR([KDIR],[Path to a kernel build tree to build against])
AC_ARG_VAR([KVER],[Kernel devel version to build against])
])

AC_DEFUN([AM_LIB_KTF],
[
libsuffix="${libdir##*/}"

# Also look for dependencies below --prefix, if set:
#
AS_IF([test "x$prefix" != "x" ],[export PKG_CONFIG_PATH=$prefix/$libsuffix/pkgconfig])
PKG_CHECK_MODULES(GTEST, gtest >= 1.9.0, [HAVE_GTEST="yes"])

ktf_build="$(pwd)/../ktf"
ktf_src="$ac_confdir/../ktf"

AC_ARG_WITH([ktf],
        [AS_HELP_STRING([--with-ktf],
        [where to find the ktf utility binaries])],
        [ktf_build=$with_ktf],
	[])

dnl deduce source directory from build directory - note the [] to avoid
dnl that M4 expands $2. Also if src is relative to build, convert to abs path:
AS_IF([test -f $ktf_build/config.log],
   [ktf_configure=$(awk '/^  \\$ .*configure/ { print $[]2; }' $ktf_build/config.log)

    AS_IF([test "x$ktf_configure" = "x./configure" ],dnl
          [
	    ktf_build=$(cd $ktf_build && pwd)
	    ktf_src="$ktf_build"
	  ],[
	    ktf_src=$(dirname $ktf_configure)
	    ktf_src=$(cd $ktf_build && cd $ktf_src && pwd)
	  ])
	  ktf_dir=$ktf_src/kernel
	  ktf_bdir=$ktf_build/kernel
   ],
   [ktf_src=$ktf_build
    ktf_dir=$ktf_build/include/ktf
    ktf_bdir=$ktf_dir
    ktf_scripts=$ktf_dir
   ]
)

AC_SUBST([KTF_DIR],[$ktf_dir])
AC_SUBST([KTF_BDIR],[$ktf_bdir])

PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.1, [have_libnl3=yes],[ dnl
  have_libnl3=no
  PKG_CHECK_MODULES([NETLINK], [libnl-1 >= 1.1])
])

AS_IF([test "${have_libnl3}" = "yes"],[
        NETLINK_CFLAGS+=" $LIBNL3_CFLAGS"
        NETLINK_LIBS+=" $LIBNL3_LIBS -lnl-genl-3"
	AC_DEFINE([HAVE_LIBNL3], 1, [Using netlink v.3])
])

KTF_CFLAGS="-I$ktf_src/lib $GTEST_CFLAGS"
KTF_LIBS="-L$ktf_build/lib -lktf $GTEST_LIBS $NETLINK_LIBS"

AC_ARG_VAR([KTF_CFLAGS],[Include files options needed for C/C++ user space program clients])
AC_ARG_VAR([KTF_LIBS],[Library options for tests accessing KTF functionality])

AC_CHECK_KPATH
])

AC_DEFUN([AM_CONFIG_KTF],
[
dnl detect any multilib architecture suffix set with --libdir
dnl Some distros use lib64 on x86_64, in which case even dependencies
dnl may be found in ${prefix}/lib64 instead of in ${prefix}/lib
dnl
libsuffix="${libdir##*/}"

AS_IF([test "x$gtest_prefix" != "x" ],[export PKG_CONFIG_PATH=$gtest_prefix/$libsuffix/pkgconfig])
PKG_CHECK_MODULES(GTEST, gtest >= 1.9.0, [HAVE_GTEST="yes"])

dnl --------------
dnl Check for our Gtest enhancement to print assert counters:
have_assert_count=0
assert_count_result="no"
dummy=_cntchk$$

AC_MSG_CHECKING([for assert counters])
cat <<EOT > $dummy.c
#include <gtest/gtest.h>
int main(int argc, char** argv) {
  size_t x = ::testing::UnitTest::GetInstance()->increment_success_assert_count();
  return 0;
}
EOT

$CXX -Isrc $CPPFLAGS $GTEST_CFLAGS -c $dummy.c -o $dummy.o > /dev/null 2>&1
if test $? = 0; then
    have_assert_count=1
    assert_count_result="yes"
fi
rm -f $dummy.o $dummy.c
AC_SUBST([HAVE_ASSERT_COUNT],[$have_assert_count])
AC_MSG_RESULT([$assert_count_result])
dnl ---------------

AS_IF([test "x${ac_confdir%%/*}" = "x." ],
    [srcdir=$(cd $srcdir; pwd)])

KTF_DIR="$srcdir/kernel"
KTF_BDIR="$(pwd)/kernel"

ktf_scripts="$srcdir/scripts"

AC_SUBST([HAVE_GTEST])
AM_CONDITIONAL([HAVE_GTEST],[test "x$HAVE_GTEST" = "xyes"])

AC_SUBST([KTF_DIR],[$KTF_DIR])
AC_SUBST([KTF_BDIR],[$KTF_BDIR])

AC_CHECK_KPATH
])

AC_DEFUN([AM_KTF_DIR],dnl Usage: AM_KTF_DIR([subdir]) where subdir contains kernel test defs
[
subdir="$1"

AS_IF([test "x${ac_confdir%%/*}" = "x." ],
	[TEST_DIR="."],
	[TEST_DIR="$srcdir/$subdir"
	 TEST_SRC=$(cd $TEST_DIR && ls *.h *.c *.S 2> /dev/null | tr '\n' ' '| sed 's/ \w*\.mod\.c|\w*version.c|\wversioninfo.h//')])

dnl Provide automatic generation of internal symbol resolving from ktf_syms.txt
dnl if it exists:
dnl
ktf_symfile=$(cd $srcdir/$subdir && ls ktf_syms.txt 2> /dev/null || true)

rulefile="$subdir/ktf_gen.mk"
top_builddir="$(pwd)"

mkdir -p $subdir
cat - > $rulefile <<EOF

top_builddir = $top_builddir
srcdir = $TEST_DIR
src_links = $TEST_SRC
ktf_symfile = $ktf_symfile

ktf_syms = \$(ktf_symfile:%.txt=%.h)
ktf_scripts = $ktf_scripts

obj-installed = \$(obj-m:%.o=$prefix/kernel/%.ko)

all: \$(ktf_syms) \$(src_links) module

Makefile: \$(srcdir)/Makefile.in \$(top_builddir)/config.status
	@case '\$?' in \\
	  *config.status*) \\
	    cd \$(top_builddir) && \$(MAKE) \$(AM_MAKEFLAGS) am--refresh;; \\
	  *) \\
	    echo ' cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@; \
	    cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@ ;; \\
	esac;

ktf_syms.h: \$(srcdir)/ktf_syms.txt
	\$(ktf_scripts)/resolve \$(ccflags-y) \$< \$[]@

install: \$(obj-installed)

\$(obj-installed): $prefix/kernel/%: %
	@(test -d $prefix/kernel || mkdir -p $prefix/kernel)
	cp \$< \$[]@

uninstall:
	@rm -f \$(src_links)

\$(src_links): %: \$(srcdir)/% Makefile
	@(test -e \$[]@ || ln -s \$< \$[]@)

EOF

])
