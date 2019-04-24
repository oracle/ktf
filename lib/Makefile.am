## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I$(top_srcdir) $(NETLINK_CFLAGS) $(GTEST_CFLAGS) \
		-Wall -Werror -Wno-packed-bitfield-compat -D_GNU_SOURCE
AM_CXXFLAGS = -I$(top_srcdir) $(NETLINK_CFLAGS) $(KTF_CXXFLAGS) \
		-Wall \
		-Wno-packed-bitfield-compat \
		-Wno-pointer-arith -Werror \
		-D__FILENAME__=\"`basename $<`\"

lib_LTLIBRARIES = libktf.la
libktf_la_SOURCES = ktf_int.cpp ktf_run.cpp ktf_unlproto.c ktf_debug.cpp

libktf_includedir = $(includedir)
libktf_include_HEADERS = ktf_debug.h ktf_int.h ktf.h

## Extra header files for the kernel side:

KTF_K_HDRS = \
    ktf.h \
    ktf_test.h \
    ktf_map.h \
    ktf_override.h \
    ktf_nl.h \
    ktf_unlproto.h \
    ktf_netctx.h \
    ktf_compat.h

kernel_headers_src = $(KTF_K_HDRS:%=$(top_srcdir)/kernel/%)
kernel_headers = $(KTF_K_HDRS:%=$(includedir)/ktf/%)
symvers = $(includedir)/ktf/Module.symvers
scripts = $(includedir)/ktf/resolve

$(kernel_headers): $(includedir)/ktf/%: $(top_srcdir)/kernel/%
	@mkdir -p `dirname $@`
	$(INSTALL) $< $@

$(symvers): $(includedir)/ktf/%: $(top_builddir)/kernel/%
	@mkdir -p `dirname $@`
	$(INSTALL) $< $@

$(scripts): $(includedir)/ktf/%: $(top_builddir)/scripts/%
	@mkdir -p `dirname $@`
	$(INSTALL) $< $@

install-data-local: $(kernel_headers) $(symvers) $(scripts)


echo:
	@echo $(kernel_headers) $(symvers)
