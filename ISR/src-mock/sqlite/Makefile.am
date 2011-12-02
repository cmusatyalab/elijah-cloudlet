AM_CFLAGS = $(glib_CFLAGS) $(gthread_CFLAGS)
AM_LDFLAGS = -lpthread -ldl $(glib_LIBS) $(gthread_LIBS)
pkglib_LTLIBRARIES = libisrsql.la
libisrsql_la_SOURCES = wrapper.c sql.h
libisrsql_la_LIBADD = libsqlitepriv.la

noinst_LTLIBRARIES = libsqlitepriv.la
libsqlitepriv_la_SOURCES = sqlite3.c sqlite3.h
# Suppress build warnings, since we're not in a position to fix them
libsqlitepriv_la_CFLAGS = -w $(VISIBILITY_HIDDEN)
