AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
AM_CFLAGS = -W -Wall -Wstrict-prototypes

GEN = viewer

if WANT_CLIENT
CLIENTPROGS = dirtometer
pkgdata_SCRIPTS = viewer
endif

if WANT_SERVER
SERVERPROGS = disktool
endif

pkglib_PROGRAMS = query blobtool $(CLIENTPROGS) $(SERVERPROGS)
EXTRA_PROGRAMS = hoardtest
CLEANFILES = $(GEN) hoardtest
EXTRA_DIST = $(GEN:=.in)

blobtool_CFLAGS = $(AM_CFLAGS) $(glib_CFLAGS) $(NO_FIELD_INITIALIZER_WARNINGS)
blobtool_LDFLAGS = -lisrcrypto $(glib_LIBS) -larchive
disktool_CFLAGS = $(AM_CFLAGS) $(glib_CFLAGS) $(NO_FIELD_INITIALIZER_WARNINGS)
disktool_LDFLAGS = $(glib_LIBS) -lisrsql -lisrutil
dirtometer_CFLAGS = $(AM_CFLAGS) $(gtk_CFLAGS) $(glib_CFLAGS)
dirtometer_CFLAGS += -Wno-unused-parameter $(NO_FIELD_INITIALIZER_WARNINGS)
dirtometer_CFLAGS += -DSHAREDIR=\"$(pkgdatadir)\"
dirtometer_LDFLAGS = $(gtk_LIBS) $(glib_LIBS)
query_CFLAGS = $(AM_CFLAGS) $(glib_CFLAGS)
query_LDFLAGS = $(glib_LIBS) -lisrsql
hoardtest_CFLAGS = $(AM_CFLAGS) $(glib_CFLAGS)
hoardtest_LDFLAGS = $(glib_LIBS) -lisrcrypto -lisrsql

include $(top_srcdir)/mkrevision.mk
include $(top_srcdir)/subst.mk
