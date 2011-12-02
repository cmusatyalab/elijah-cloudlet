AM_CFLAGS  = -W -Wall -Wstrict-prototypes -funroll-loops -fomit-frame-pointer
AM_CFLAGS += $(VISIBILITY_HIDDEN) $(glib_CFLAGS) $(liblzma_CFLAGS)
# autoconf puts a -O2 flag in CFLAGS which cannot be overridden by AM_CFLAGS
CFLAGS=@CFLAGS@ -O3
AM_LDFLAGS = -lz $(glib_LIBS) $(liblzma_LIBS)

pkglib_LTLIBRARIES = libisrcrypto.la
libisrcrypto_la_SOURCES = aes.c cbc.c cipher.c compress.c
libisrcrypto_la_SOURCES += ecb.c hash.c hmac.c lzf.c lzf-stream.c lzma.c
libisrcrypto_la_SOURCES += mac.c md5.c md5-compress.c pad.c random.c
libisrcrypto_la_SOURCES += sha1.c util.c zlib.c
libisrcrypto_la_SOURCES += isrcrypto.h internal.h aes_tab.h

if HAVE_X86_64
libisrcrypto_la_SOURCES += sha1-compress-amd64.S
else
if HAVE_X86_32
libisrcrypto_la_SOURCES += sha1-compress-ia32.S
else
libisrcrypto_la_SOURCES += sha1-compress.c
endif
endif

SUBDIRS = . test
