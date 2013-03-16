ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
AM_CFLAGS = -std=gnu99 -W -Wall -Wstrict-prototypes -pthread \
	$(libcurl_CFLAGS) $(glib_CFLAGS) $(gthread_CFLAGS) $(fuse_CFLAGS)
AM_LDFLAGS = -pthread $(libcurl_LIBS) $(glib_LIBS) $(gthread_LIBS) \
	$(fuse_LIBS)

pkglibexec_PROGRAMS = vmnetfs/vmnetfs
vmnetfs_vmnetfs_SOURCES  = vmnetfs/bitmap.c vmnetfs/cond.c vmnetfs/fuse.c \
	vmnetfs/fuse-image.c vmnetfs/fuse-stats.c vmnetfs/fuse-stream.c \
	vmnetfs/io.c vmnetfs/ll-modified.c vmnetfs/ll-pristine.c \
	vmnetfs/pollable.c vmnetfs/stats.c vmnetfs/stream.c \
	vmnetfs/transport.c vmnetfs/util.c vmnetfs/vmnetfs.c \
	vmnetfs/vmnetfs-private.h


