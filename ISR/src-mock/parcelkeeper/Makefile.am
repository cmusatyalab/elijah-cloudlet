AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
AM_CFLAGS = -W -Wall -Wstrict-prototypes -Wno-unused-parameter
AM_CFLAGS += $(NO_FIELD_INITIALIZER_WARNINGS)
AM_CFLAGS += $(libuuid_CFLAGS) $(glib_CFLAGS) $(gthread_CFLAGS) $(fuse_CFLAGS)
AM_LDFLAGS = -rdynamic -lcurl -lisrsql -lrt -lisrcrypto -lisrutil
AM_LDFLAGS += $(libuuid_LIBS) $(glib_LIBS) $(gthread_LIBS) $(fuse_LIBS)

pkglib_PROGRAMS = parcelkeeper
parcelkeeper_SOURCES  = cmdline.c main.c log.c cache.c cache_modes.c fuse.c
parcelkeeper_SOURCES += fuse_image.c fuse_stats.c fuse_defs.h
parcelkeeper_SOURCES += hoard.c hoard_modes.c util.c parcelcfg.c
parcelkeeper_SOURCES += transport.c defs.h
nodist_parcelkeeper_SOURCES = revision.c
CLEANFILES = revision.c

include $(top_srcdir)/mkrevision.mk
