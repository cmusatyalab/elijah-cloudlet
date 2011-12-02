include $(top_srcdir)/vmm.mk

DUMMY_VMMS = shell none
GEN = $(DUMMY_VMMS) $(ALL_VMMS)
CLEANFILES = $(GEN) vmm.pyc
EXTRA_DIST = $(GEN:=.in)
vmmdir = $(pkgdatadir)/vmm

if WANT_CLIENT
CLIENT_VMMS = $(DUMMY_VMMS)
dist_pkgdata_DATA = vmm.pm vmm.py
endif

vmm_SCRIPTS = $(CLIENT_VMMS) $(VMMS)
EXTRA_SCRIPTS = $(ALL_VMMS)

include $(top_srcdir)/mkrevision.mk
include $(top_srcdir)/subst.mk
