# Makefile fragment to autogenerate revision.[ch]

top_srcdir ?= ..
top_builddir ?= $(top_srcdir)

# This is a hack.  The include directive forces make to always execute
# the rule, and to recalculate its target selection afterward.
.PHONY: revision.dummy
revision.dummy:
	@$(top_srcdir)/mkrevision.sh $(top_builddir) update
-include revision.dummy

REVISION_FILE = $(top_srcdir)/.gitrevision
REVISION_DEPENDS = $(REVISION_FILE) $(top_builddir)/config.h
RCS_REVISION = $(shell cat $(REVISION_FILE))

revision.c: $(REVISION_DEPENDS)
	$(top_srcdir)/mkrevision.sh $(top_builddir) object

revision.h: $(REVISION_DEPENDS)
	$(top_srcdir)/mkrevision.sh $(top_builddir) header
