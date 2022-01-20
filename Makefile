MKPATH=mk/
include $(MKPATH)buildsys.mk

SUBDIRS = src lib
CLEAN = *.dll *.exe
DISTCLEAN = config.status config.log docs/.deps \
	mk/buildsys.mk mk/extra.mk
REPOCLEAN = aclocal.m4 autom4te.cache configure src/autoconf.h.in version

.PHONY: tests manual manual-optional dist
tests:
	$(MAKE) -C src tests

TAG = beleriand-`cd scripts && ./version.sh`
OUT = $(TAG).tar.gz

all: manual-optional

manual: manual-optional
	@if test x"$(SPHINXBUILD)" = x || test x"$(SPHINXBUILD)" = xNOTFOUND ; then \
		echo "sphinx-build was not found during configuration.  If it is not installed, you will have to install it.  You can either rerun the configuration or set SPHINXBUILD on the command line when running make to inform make how to run sphinx-build.  You may also want to set DOC_HTML_THEME to a builtin Sphinx theme to use instead of what is configured in docs/conf.py.  For instance, 'DOC_HTML_THEME=classic'." ; \
		exit 1 ; \
	fi

manual-optional:
	@if test ! x"$(SPHINXBUILD)" = x && test ! x"$(SPHINXBUILD)" = xNOTFOUND ; then \
		env DOC_HTML_THEME="$(DOC_HTML_THEME)" $(MAKE) -C docs SPHINXBUILD="$(SPHINXBUILD)" html ; \
	fi

dist:
	git checkout-index --prefix=$(TAG)/ -a
	scripts/version.sh > $(TAG)/version
	$(TAG)/autogen.sh
	rm -rf $(TAG)/autogen.sh $(TAG)/autom4te.cache
	tar --exclude .gitignore --exclude *.dll --exclude .github \
		--exclude .travis.yml -czvf $(OUT) $(TAG)
	rm -rf $(TAG)

# If this isn't a --with-no-install build, build and install the documentation
# if sphinx-build is available.
install-extra:
	@if test x"$(NOINSTALL)" != xyes && test ! x"$(SPHINXBUILD)" = x && test ! x"$(SPHINXBUILD)" = xNOTFOUND ; then \
		for x in `find docs/_build/html -mindepth 1 \! -type d \! -name .buildinfo -print`; do \
			i=`echo $$x | sed -e s%^docs/_build/html/%%`; \
			${INSTALL_STATUS}; \
			if ${MKDIR_P} $$(dirname ${DESTDIR}${docdatadir}/$$i) && ${INSTALL} -m 644 $$x ${DESTDIR}${docdatadir}/$$i; then \
				${INSTALL_OK}; \
			else \
				${INSTALL_FAILED}; \
			fi \
		done \
	fi

# Hack to clean up in docs since it isn't included in SUBDIRS.
pre-clean:
	@if test ! x"$(SPHINXBUILD)" = x && test ! x"$(SPHINXBUILD)" = xNOTFOUND ; then \
		$(MAKE) -C docs SPHINXBUILD="$(SPHINXBUILD)" clean ; \
	fi

# Hack to clean up test results in tests.
pre-distclean:
	@find tests -name run.out -exec rm {} \;

# Remove the files generated by autogen.sh and the version stamp file.
# In general, this should not be used when working with a distributed
# source archive (.tar.gz) since those archives do not include autogen.sh for
# rebuilding the configure script.
repoclean: distclean
	for i in "" $(REPOCLEAN) ; do \
		test x"$$i" = x"" && continue; \
		if test -d "$$i" || test -f "$$i" ; then rm -rf "$$i" ; fi \
	done
