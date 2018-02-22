## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS= -I m4

SUBDIRS = kernel selftest examples lib user doc

documentation htmlhelp htmldocs:
	@(cd doc && $(MAKE) htmlhelp)
