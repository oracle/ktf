# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
SPHINXPROJ    = KernelTestFramework
SPHINXSTATIC  = sphinx-static
SOURCEDIR     = $(srcdir)
BUILDDIR      = .
TARGETS	      = htmlhelp

docs: htmldocs

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

# silence Spinx completely unless V=1
# For some reason the htmlhelp target does not produce _sources, while
# the html target does not produce correct next and prev links
# Make both and sync the _sources over
#
htmldocs: $(TARGETS)

$(TARGETS): Makefile $(srcdir)/*.rst
	@if test x$V != x1; then export Q="-q"; echo -n "  HTMLDOC $$(pwd)"; fi; \
	$(SPHINXBUILD) -M html "$(SOURCEDIR)" "$(BUILDDIR)" $$Q $(SPHINXOPTS) $(O) | \
	if test x$V != x1; then grep -v 'Build finished'; else cat -; fi; \
	$(SPHINXBUILD) -M htmlhelp "$(SOURCEDIR)" "$(BUILDDIR)" $$Q $(SPHINXOPTS) $(O) | \
	if test x$V != x1; then grep -v 'Build finished' || echo ""; else cat -; fi
	@rsync -a html/_sources htmlhelp/

clean-local:
	rm -fr doctrees htmlhelp
