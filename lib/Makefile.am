## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) -Wall -Werror -Wno-packed-bitfield-compat -D_GNU_SOURCE
AM_CXXFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) \
		-Wall \
		-Wno-packed-bitfield-compat \
		-Wno-pointer-arith -Werror \
		-D__FILENAME__=\"`basename $<`\"

lib_LTLIBRARIES = libktf.la
libktf_la_SOURCES = utest.cpp ktf_run.cpp unlproto.c debug.cpp

## Simple kernel test runner sample program:
#ktf_SOURCES = ktf.cpp
#ktf_LDADD = -lktf $(NETLINK_LIBS) $(GTEST_LIBS)
