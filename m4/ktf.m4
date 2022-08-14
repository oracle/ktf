# SPDX-License-Identifier: GPL-2.0
#
# Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
#    Author: Knut Omang <knut.omang@oracle.com>
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

AS_IF([ test "x$KVER" = x -a "x$KDIR" = x ],dnl
	[AC_MSG_ERROR([dnl
 Neither environment KVER nor KDIR is set!
 - Set KVER to an installed kernel version (for instance as output by "uname -r")
   Alternatively set KDIR to the directory of the toplevel Makefile of a kernel build tree])]
)

AC_ARG_VAR([KDIR],[Path to a kernel build tree to build against])
AC_ARG_VAR([KVER],[Kernel devel version to build against])
])


AC_DEFUN([AC_CHECK_CXXFLAGS],
[
AC_LANG_PUSH([C++])
dnl Enable C++11 if possible. If it cannot be enabled,
dnl googletest would have had to be compiled without it too, implying
dnl an older version where the flag was named c++0x, if existing at all:
AX_CHECK_COMPILE_FLAG([-std=c++11],
	[KTF_CXXFLAGS="-std=c++11 $KTF_CXXFLAGS"],
	[KTF_CXXFLAGS="-std=c++0x $KTF_CXXFLAGS"])
AC_LANG_POP([C++])
])


AC_DEFUN([AC_CHECK_NETLINK],
[
PKG_CHECK_MODULES(LIBNL3, [libnl-3.0 >= 3.1], [have_libnl3=yes],[ dnl
  have_libnl3=no
  PKG_CHECK_MODULES([NETLINK], [libnl-1 >= 1.1])
])

if (test "${have_libnl3}" = "yes"); then
        NETLINK_CFLAGS+=" $LIBNL3_CFLAGS"
        NETLINK_LIBS+=" $LIBNL3_LIBS -lnl-genl-3"
	AC_DEFINE([HAVE_LIBNL3], 1, [Using netlink v.3])
else
	# libnl does not define NLA_BINARY, but kernels since v2.6.21 supports it
	# and no special user side handling is needed:
	NETLINK_CFLAGS+="-DNLA_BINARY=11"
fi
])


AC_DEFUN([AM_LIB_KTF],
[

AC_CHECK_LIBDIR

# Also look for dependencies below --prefix, if set:
#
AS_IF([test "x$prefix" != "x" ],[export PKG_CONFIG_PATH=$prefix/$libsuffix/pkgconfig])
PKG_CHECK_MODULES(GTEST, [gtest >= 1.9.0], [HAVE_GTEST="yes"])

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
   [ktf_configure=$(awk '/^  \$[] .*configure/ { print $[]2; }' $ktf_build/config.log)

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

AC_CHECK_NETLINK

KTF_CFLAGS="-I$ktf_src/lib $GTEST_CFLAGS"
KTF_CXXFLAGS="-I$ktf_src/lib $GTEST_CFLAGS"
KTF_LIBS="-L$ktf_build/lib -lktf $GTEST_LIBS $NETLINK_LIBS"

dnl optionally set the -std=c++11 flag for c++ (see def, above):
AC_CHECK_CXXFLAGS

AC_ARG_VAR([KTF_CFLAGS],[Include files options needed for C user space program clients])
AC_ARG_VAR([KTF_CXXFLAGS],[Include files options needed for C++ user space program clients])
AC_ARG_VAR([KTF_LIBS],[Library options for tests accessing KTF functionality])

AC_CHECK_KPATH
])


dnl Check for the Oracle Gtest enhancement to print assert counters:
dnl (not necessary for the operation of KTF, but useful as an
dnl  additional debugging measure)
dnl
AC_DEFUN([AC_CHECK_ASSERT_CNT],
[
have_assert_count=0
assert_count_result="no"
dummy=_cntchk$$

AC_MSG_CHECKING([for assert counters])
cat <<EOT > $dummy.c
#include <gtest/gtest.h>
int main(int argc, char** argv) {
  size_t x = ::testing::UnitTest::GetInstance()->increment_success_assert_count();
  (void)x;
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
AC_DEFINE_UNQUOTED([HAVE_ASSERT_COUNT],$have_assert_count)
AC_MSG_RESULT([$assert_count_result])
])


dnl Check whether ::testing::internal::ParameterizedTestCaseInfo<T>::AddTestPattern
dnl has got a CodeLocation argument.
dnl this was introduced to Googletest without any versioning protection after v.1.10.0.
dnl
AC_DEFUN([AC_CHECK_CODELOC_ARG_FOR_ADDTESTPATTERN],
[
have_codeloc_for_addtestpattern=0
cl_result="no"
assert_count_result="no"
dummy=_codelocargchk$$

AC_MSG_CHECKING([whether gtest ParameterizedTestCaseInfo<T>::AddTestPattern has got a 4th argument])
cat <<EOT > $dummy.c
#include <gtest/gtest.h>
#include <string>

class T
{
public:
  typedef std::string ParamType;
};

void f(::testing::internal::ParameterizedTestCaseInfo<T>* foo)
{
     foo->AddTestPattern("a", "b", NULL, ::testing::internal::CodeLocation("", 0));
}
EOT

$CXX -Isrc $CPPFLAGS $GTEST_CFLAGS -c $dummy.c -o $dummy.o > /dev/null 2>&1
if test $? = 0; then
    have_codeloc_for_addtestpattern=1
    cl_result="yes"
fi
rm -f $dummy.o $dummy.c
AC_SUBST([HAVE_CODELOC_FOR_ADDTESTPATTERN],[$have_codeloc_for_addtestpattern])
AC_DEFINE_UNQUOTED([HAVE_CODELOC_FOR_ADDTESTPATTERN],$have_codeloc_for_addtestpattern)
AC_MSG_RESULT([$cl_result])
])


AC_DEFUN([AC_CHECK_LIBDIR],
[
dnl detect any multilib architecture suffix set with --libdir
dnl Some distros use lib64 on x86_64, in which case even dependencies
dnl may be found in ${prefix}/lib64 instead of in ${prefix}/lib
dnl
libsuffix="${libdir##*/}"

dnl Avoid having to specify --libdir explicitly on x86_64 where it is needed:
dnl
lib64_paths=$(egrep -v '^#' /etc/ld.so.conf.d/* /etc/ld.so.conf | grep lib64)
arch=$(arch)
AS_IF([test "x$arch" = "xx86_64" -a "x$lib64_paths" != "x" -a "x$libsuffix" = "xlib" ],
[
    libsuffix="lib64"
    libdir="${libdir}64"
])
])


dnl This macro is an internal helper to configure KTF itself.
dnl It is somewhat different from AM_LIB_KTF, used by clients to
dnl configure *use* of KTF:
dnl
AC_DEFUN([AM_CONFIG_KTF],
[

AC_CHECK_LIBDIR

AS_IF([test "x$gtest_prefix" != "x" ],[export PKG_CONFIG_PATH=$gtest_prefix/$libsuffix/pkgconfig])
PKG_CHECK_MODULES(GTEST, [gtest >= 1.9.0], [HAVE_GTEST="yes"])

AC_CHECK_ASSERT_CNT
AC_CHECK_CODELOC_ARG_FOR_ADDTESTPATTERN

AS_IF([test "x${ac_confdir%%/*}" = "x." ],
    [srcdir=$(cd $srcdir; pwd)])

KTF_DIR="$srcdir/kernel"
KTF_BDIR="$(pwd)/kernel"

ktf_scripts="$srcdir/scripts"

AC_SUBST([HAVE_GTEST])
AM_CONDITIONAL([HAVE_GTEST],[test "x$HAVE_GTEST" = "xyes"])

AC_SUBST([KTF_DIR],[$KTF_DIR])
AC_SUBST([KTF_BDIR],[$KTF_BDIR])

AC_CHECK_NETLINK

# Internal client directories need this:
#
KTF_CFLAGS="-I$srcdir/lib $GTEST_CFLAGS"
KTF_CXXFLAGS="-I$srcdir/lib $GTEST_CFLAGS"
KTF_LIBS="-L$(pwd)/lib -lktf $GTEST_LIBS $NETLINK_LIBS"

dnl optionally set the -std=c++11 flag for c++ (see def, above):
AC_CHECK_CXXFLAGS

AC_ARG_VAR([KTF_CFLAGS],[Include files options needed for C user space program clients])
AC_ARG_VAR([KTF_CXXFLAGS],[Include files options needed for C++ user space program clients])
AC_ARG_VAR([KTF_LIBS],[Library options for tests accessing KTF functionality])

AC_CHECK_KPATH
])

AC_DEFUN([AM_KTF_DIR],dnl Usage: AM_KTF_DIR([subdir]) where subdir contains kernel test defs
[
subdir="$1"

AS_IF([test "x${ac_confdir%%/*}" = "x." ],
	[TEST_DIR="."],
	[TEST_DIR="$srcdir/$subdir"
	 TEST_SRC=$(cd $TEST_DIR && ls *.h *.c *.S runchecks.cfg 2> /dev/null | tr '\n' ' '| sed 's/ \w*\.mod\.c|\w*version.c|\wversioninfo.h//')])

dnl Provide automatic generation of internal symbol resolving from ktf_syms.txt
dnl if it exists:
dnl
ktf_symfile=$(cd $srcdir/$subdir && ls ktf_syms.txt 2> /dev/null || true)

rulefile="$subdir/ktf_gen.mk"
top_builddir="$(pwd)"

mkdir -p $subdir
cat - > $rulefile <<EOF

top_builddir = $top_builddir
subdir = $subdir
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
	    echo ' cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@'; \
	    cd \$(top_builddir) && \$(SHELL) ./config.status \$(subdir)/\$[]@ ;; \\
	esac;

ktf_syms.h: \$(srcdir)/ktf_syms.txt \$(ktf_scripts)/resolve
	\$(ktf_scripts)/resolve -I\$(srcdir) \$(ccflags-y) \$< \$[]@

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
