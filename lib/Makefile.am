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

libktf_includedir = $(includedir)
libktf_include_HEADERS = ktf_run.h utest.h

## Extra header files for the kernel side:

KTF_K_HDRS = \
    ktf.h \
    kcheck.h \
    ktf_map.h \
    nl.h

kernel_headers_src = $(KTF_K_HDRS:%=$(top_srcdir)/kernel/%)
kernel_headers = $(KTF_K_HDRS:%=$(includedir)/ktf/%)
symvers = $(includedir)/ktf/Module.symvers

$(kernel_headers): $(includedir)/ktf/%: $(top_srcdir)/kernel/%
	@mkdir -p `dirname $@`
	$(INSTALL) $< $@

$(symvers): $(includedir)/ktf/%: $(top_builddir)/kernel/%
	@mkdir -p `dirname $@`
	$(INSTALL) $< $@

install-data-local: $(kernel_headers) $(symvers)


echo:
	@echo $(kernel_headers) $(symvers)
