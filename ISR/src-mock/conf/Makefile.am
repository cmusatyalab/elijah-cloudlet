completiondir = ${sysconfdir}/bash_completion.d

dist_pkgdata_DATA = config logo-128.png
completion_DATA = openisr-client.completion

# Dummy variable to cause the directory to be created
dist_pkgsysconf_DATA = 

GEN = openisr-client.completion
CLEANFILES = $(GEN)
EXTRA_DIST = $(GEN:=.in) logo.svg magic

install-data-hook:
	cd $(DESTDIR)$(completiondir) && \
				mv openisr-client.completion openisr-client

uninstall-hook:
	rm -f $(DESTDIR)$(completiondir)/openisr-client

include $(top_srcdir)/mkrevision.mk
include $(top_srcdir)/subst.mk
