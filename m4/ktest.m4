# Require ktest and its dependencies:
# AM_LIB_KTEST
#
AC_DEFUN([AM_LIB_KTEST],
[

ktest_build="`pwd`/../ktest"
ktest_src="$ac_confdir/../ktest"

AC_ARG_WITH([ktest],
        [AS_HELP_STRING([--with-ktest],
        [where to find the ktest utility binaries])],
        [ktest_build=$with_ktest],
	[])

dnl deduce source directory from build directory - note the ][ to avoid
dnl that M4 expands $2. Also if src is relative to build, convert to abs path:
AS_IF([test -f $ktest_build/config.log],
   [ktest_configure=`awk '/^  \\$ .*configure/ { print $[]2; }' $ktest_build/config.log`

    AS_IF([test "x$ktest_configure" = "x./configure" ],dnl
          [ktest_src="$ktest_build"],dnl
          [
	    ktest_src=`dirname $ktest_configure`
	    ktest_src=`cd $ktest_build && cd $ktest_src && pwd`
	  ])
   ],
   [ktest_src=$ktest_build]
)

AC_SUBST([KTEST_DIR],[$ktest_src/kernel])
AC_SUBST([KTEST_BDIR],[$ktest_build/kernel])

KTEST_CFLAGS="-I$ktest_src/lib"
KTEST_LIBS="-L$ktest_build/lib -lktest"

AC_ARG_VAR([KTEST_CFLAGS],[Include files options needed for C/C++ user space program clients])
AC_ARG_VAR([KTEST_LIBS],[Library options for tests accessing KTEST functionality])

])

AC_DEFUN([AM_CONFIG_KTEST],
[

gtest_fail_msg="No gtest library - install gtest-devel"

GTEST_LIB_CHECK([1.5.0],[echo -n ""],[AC_MSG_ERROR([$gtest_fail_msg])])

KTEST_DIR="$srcdir/kernel"
KTEST_BDIR="`pwd`/kernel"

AC_SUBST([KTEST_DIR],[$KTEST_DIR])
AC_SUBST([KTEST_BDIR],[$KTEST_BDIR])
])

AC_DEFUN([AM_KTEST_DIR],dnl Usage: AM_KTEST_DIR([subdir]) where subdir contains kernel test defs
[

TEST_DIR="$srcdir/$1"
TEST_SRC=`cd $TEST_DIR && ls *.h *.c *.S 2> /dev/null | tr '\n' ' '| sed 's/ \w*\.mod\.c|\w*version.c|\wversioninfo.h//'`


rulepath="$1"
rulefile="$rulepath/genrules.mk"

mkdir -p $rulepath
cat - > $rulefile <<EOF

srcdir= $TEST_DIR
src_links= $TEST_SRC

all: \$(src_links) module

install: all
uninstall:
	@rm -f \$(src_links)

\$(src_links): %: \$(srcdir)/% Makefile
	@(test -e \$[]@ || ln -s \$< \$[]@)

EOF

])
