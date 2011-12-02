# Makefile fragment to generate the text files in $(GEN) from filename.in,
# substituting various build variables.  mkrevision.mk must have already
# been included.

$(GEN): %: %.in Makefile $(REVISION_DEPENDS)
	@echo "Generating $@ from $<"
	@sed -e "s=!!LIBDIR!!=$(pkglibdir)=g" \
				-e "s=!!SHAREDIR!!=$(pkgdatadir)=g" \
				-e "s=!!SYSCONFDIR!!=$(pkgsysconfdir)=g" \
				-e "s=!!BINDIR!!=$(bindir)=g" \
				-e "s=!!SBINDIR!!=$(sbindir)=g" \
				-e "s=!!STATEDIR!!=$(pkgstatedir)=g" \
				-e "s=!!PERLPATH!!=$(PERL)=g" \
				-e "s=!!PYTHONPATH!!=$(PYTHON)=g" \
				-e "s=!!VERSION!!=$(PACKAGE_VERSION)=g" \
				-e "s=!!REVISION!!=$(RCS_REVISION)=g" $< > $@
