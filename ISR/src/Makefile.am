if WANT_CLIENT
CLIENTDIRS = client parcelkeeper conf
endif

if WANT_SERVER
SERVERDIRS = locksrv
endif

if WANT_VMMS
VMMDIRS = vmm
endif

if WANT_COMMON
COMMONDIRS = crypto sqlite libutil tools
endif

SUBDIRS = $(COMMONDIRS) $(CLIENTDIRS) $(SERVERDIRS) $(VMMDIRS)
dist_noinst_SCRIPTS = mkrevision.sh autogen.sh
EXTRA_DIST  = LICENSE.Eclipse LICENSE.GPL LICENSE.LGPL CHANGES README.server
EXTRA_DIST += .gitrevision

ACLOCAL_AMFLAGS = -I m4
# "make distcheck" should check as much as possible.
DISTCHECK_CONFIGURE_FLAGS = --enable-server

# This is marked PHONY so that it will run unconditionally during "make dist";
# otherwise, we might distribute an obsolete .gitrevision.
.PHONY: .gitrevision
.gitrevision:
	$(top_srcdir)/mkrevision.sh $(top_builddir) update
