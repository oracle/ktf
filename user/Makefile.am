## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir)/lib $(NETLINK_CFLAGS) $(KTF_CFLAGS) \
		-Wall -Werror \
		-Wno-packed-bitfield-compat -D_GNU_SOURCE

AM_CXXFLAGS = -I$(top_srcdir)/lib $(NETLINK_CFLAGS) $(KTF_CXXFLAGS) \
		-Wall \
		-Wno-packed-bitfield-compat \
		-Wno-pointer-arith -Werror \
		-D__FILENAME__=\"`basename $<`\"
LDADD =	-L$(top_builddir)/lib -lktf $(NETLINK_LIBS) $(KTF_LIBS)

bin_PROGRAMS = ktfrun ktfcov hybrun

## Simple kernel test runner sample program:
ktfrun_SOURCES = ktfrun.cpp
ktfcov_SOURCES = ktfcov.cpp

## A mixed kernel and user level test program:
hybrun_SOURCES = ktfrun.cpp hybrid.cpp
