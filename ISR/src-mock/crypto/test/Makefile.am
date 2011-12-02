AM_CFLAGS = -W -Wall -Wstrict-prototypes -Wno-unused-parameter
AM_LDFLAGS = -lisrcrypto

TESTS = run_vectors
check_PROGRAMS = $(TESTS)
run_vectors_SOURCES  = run_vectors.c vectors.h vectors_aes.h
run_vectors_SOURCES += vectors_compress.h vectors_hmac.h vectors_md5.h
run_vectors_SOURCES += vectors_sha1.h

EXTRA_PROGRAMS = compress
compress_CFLAGS = $(AM_CFLAGS) $(glib_CFLAGS)
compress_LDFLAGS = $(AM_LDFLAGS) $(glib_LIBS)
