AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
AM_CFLAGS = -W -Wall -Wstrict-prototypes $(glib_CFLAGS)
AM_LDFLAGS = $(glib_LIBS)

SRVCMDS = catlog checkparcel commit getconfig lock ls motd rollback stat
SRVSCRIPTS = $(patsubst %, srv_%.pm, $(SRVCMDS))
serverstatedir = $(pkgstatedir)/server

# Dummy variable to cause directory to be created
dist_serverstate_DATA = 

bin_PROGRAMS = isr_shell
bin_SCRIPTS = isr_runserv
sbin_SCRIPTS = isr_admin
dist_pkgdata_DATA = Server.pm $(SRVSCRIPTS)

GEN = isr_runserv isr_admin
CLEANFILES = $(GEN)
EXTRA_DIST = $(GEN:=.in) README

include $(top_srcdir)/mkrevision.mk
include $(top_srcdir)/subst.mk
