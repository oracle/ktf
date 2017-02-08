## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) -Wall -Werror -Wno-packed-bitfield-compat -D_GNU_SOURCE
AM_CXXFLAGS = -I$(top_srcdir) $(LIBNL3_CFLAGS) \
		-Wall \
		-Wno-packed-bitfield-compat \
		-Wno-pointer-arith -Werror \
		-D__FILENAME__=\"`basename $<`\"

lib_LTLIBRARIES = libktest.la
libktest_la_SOURCES = utest.cpp ktest_run.cpp unlproto.c debug.cpp

## Simple kernel test runner sample program:
#ktest_SOURCES = ktest.cpp
#ktest_LDADD = -lktest $(NETLINK_LIBS) $(GTEST_LIBS)
