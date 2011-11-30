#
# configure.ac - build infrastructure for the OpenISR (R) system
#
# Copyright (C) 2007-2010 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

## Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([OpenISR], [0.10.1])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign 1.10])
AC_COPYRIGHT([Copyright (C) 2007-2010 Carnegie Mellon University])
AC_CONFIG_SRCDIR([parcelkeeper/cache.c])
AC_CONFIG_MACRO_DIR([m4])
AC_DISABLE_STATIC

# Generating a config.h prevents the introduction of several lines' worth of
# -D flags on every gcc command line
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG()

# Checks for optional features.
AC_ARG_ENABLE([client], [AS_HELP_STRING([--enable-client],
			[build client code [yes]])],, [enable_client=yes])
AC_ARG_ENABLE([vmm-drivers], [AS_HELP_STRING([--enable-vmm-drivers=<list>],
			[build VMM drivers [$client]])],,
			[enable_vmm_drivers=${enable_client}])
AC_ARG_ENABLE([server], [AS_HELP_STRING([--enable-server],
			[build server code [no]])],, [enable_server=no])

PROCESS_ENABLE_VAR([$enable_client], [WANT_CLIENT],
			[whether we should build the client])
PROCESS_ENABLE_VAR([$enable_server], [WANT_SERVER],
			[whether we should build the server])
AM_CONDITIONAL([WANT_COMMON],
			[test z$enable_client = zyes -o z$enable_server = zyes])

AC_MSG_CHECKING([which VMM drivers to build])
REQUESTED_VMMS=`echo "$enable_vmm_drivers" | sed -e 's/,/ /g'`
AC_SUBST([REQUESTED_VMMS])
export REQUESTED_VMMS
vmms=`$am_make -f $srcdir/vmm.mk --no-print-directory list_chosen_vmms`
unknown_vmms=`$am_make -f $srcdir/vmm.mk --no-print-directory list_invalid_vmms`
if test -n "$unknown_vmms" ; then
	AC_MSG_ERROR([unknown VMM drivers requested: $unknown_vmms])
fi
if test -n "$vmms" ; then
	AC_MSG_RESULT([$vmms])
else
	AC_MSG_RESULT([none])
fi
AM_CONDITIONAL([WANT_VMMS], [test -n "$vmms" || test z$enable_client = zyes])

# Checks for compiler/host CPU characteristics.
# Autoconf quadrigraphs: @<:@ is a literal '['; @:>@ is a literal ']'
case $host_cpu in
i@<:@456@:>@86)
	AC_DEFINE([HAVE_X86_32], [1], [Define to 1 if you have a 486 or better IA32 processor.])
	;;
x86_64)
	AC_DEFINE([HAVE_X86_64], [1], [Define to 1 if you have an AMD64 processor.])
	;;
esac
AM_CONDITIONAL([HAVE_X86_32], [echo $host_cpu | grep -q '^i@<:@456@:>@86$'])
AM_CONDITIONAL([HAVE_X86_64], [test z$host_cpu = zx86_64])

CHECK_COMPILER_OPTION([-fvisibility=hidden])
VISIBILITY_HIDDEN=
if test z$success = zyes ; then
	AC_DEFINE([HAVE_VISIBILITY], [1], [Define to 1 if your compiler supports the -fvisibility=hidden option.])
	VISIBILITY_HIDDEN="-fvisibility=hidden"
fi
AC_SUBST([VISIBILITY_HIDDEN])

# gcc 3 doesn't support -Wno-missing-field-initializers; the only thing we
# can do there to disable these warnings is to disable -W.  gcc 3 doesn't
# matter enough for us to provide a mechanism to disable -W for only those
# targets that care, but it'd be nice to be able to build on it.  So we'll
# just put up with lots of spurious warnings under old gcc.
CHECK_COMPILER_OPTION([-Wno-missing-field-initializers])
NO_FIELD_INITIALIZER_WARNINGS=
if test z$success = zyes ; then
	NO_FIELD_INITIALIZER_WARNINGS="-Wno-missing-field-initializers"
fi
AC_SUBST([NO_FIELD_INITIALIZER_WARNINGS])

# Checks for programs.
AC_PATH_PROG([PERL], [perl], [no])
if test z$PERL = zno ; then
	AC_MSG_ERROR([Perl not found])
fi
AC_SUBST([PERL])
AC_ARG_VAR([PERL], [path to Perl])

if test z$enable_client = zyes || WANT_VMM([virtualbox]) ; then
	AC_PATH_PROG([PYTHON], [python], [no])
	if test z$PYTHON = zno ; then
		AC_MSG_ERROR([Python not found])
	fi
	AC_SUBST([PYTHON])
	AC_ARG_VAR([PYTHON], [path to Python])
	ver=`$PYTHON -V 2>&1 | cut -f2 -d\  `
	CHECK_VERSION_VAL([python], [$ver], [2.4])
fi

# Checks for libraries.
AC_ARG_WITH([zlib], [AS_HELP_STRING([--with-zlib=DIR],
			[look for zlib in DIR])])
AC_ARG_WITH([curl], [AS_HELP_STRING([--with-curl=DIR],
			[look for libcurl in DIR])])
AC_ARG_WITH([libarchive], [AS_HELP_STRING([--with-libarchive=DIR],
			[look for libarchive in DIR])])

if test z$enable_client = zyes ; then
	# We can't use AC_CHECK_FUNCS() to check for fallocate(), because
	# we need to #define _FILE_OFFSET_BITS 64 when doing the test.
	# glibc 2.10 (Ubuntu Karmic) fails to export fallocate64(), so without
	# the #define we'd misdetect the availability of fallocate() on 32-bit
	# platforms.
	RUN_TEST([LINK], [for fallocate], [AC_LANG_PROGRAM(
				[#define _FILE_OFFSET_BITS 64
				 #define _GNU_SOURCE
				 #include <fcntl.h>],
				[return fallocate(0, 0, 0, 0);])])
	if test z$success = zyes ; then
		AC_DEFINE([HAVE_FALLOCATE], [1], [Define to 1 if you have the 'fallocate' function.])
	fi

	FIND_LIBRARY([libcurl], [curl], [curl_easy_init], [curl/curl.h],
				[$with_curl /usr/local /usr])
	if test ! -x $FOUND_PATH/bin/curl-config ; then
		AC_MSG_ERROR([cannot find curl-config])
	fi
	ver=`$FOUND_PATH/bin/curl-config --version | cut -f2 -d\  `
	CHECK_VERSION_VAL([curl], [$ver], [7.12.3])
	
	PKG_CHECK_MODULES([gtk], [gtk+-2.0 >= 2.8])
	PKG_CHECK_MODULES([libuuid], [uuid])
	PKG_CHECK_MODULES([fuse], [fuse >= 2.7])

	saved_cflags="$CFLAGS"
	CFLAGS="$CFLAGS $fuse_CFLAGS"
	RUN_TEST([COMPILE], [for flag_nullpath_ok in fuse.h],
				[AC_LANG_PROGRAM(
				[#define FUSE_USE_VERSION 26
				#include <fuse.h>],
				[struct fuse_operations ops = {
					.flag_nullpath_ok = 1
				};])])
	CFLAGS="$saved_cflags"
	if test z$success = zyes ; then
		AC_DEFINE([HAVE_FUSE_NULLPATH_OK], [1], [Define to 1 if struct fuse_operations includes flag_nullpath_ok.])
	fi
fi

if test z$enable_client = zyes -o z$enable_server = zyes ; then
	FIND_LIBRARY([zlib], [z], [inflate], [zlib.h],
				[$with_zlib /usr/local /usr])
	FIND_LIBRARY([libarchive], [archive], [archive_read_extract],
				[archive.h archive_entry.h],
				[$with_libarchive /usr/local /usr])
	PKG_CHECK_MODULES([glib], [glib-2.0 >= 2.12])
	PKG_CHECK_MODULES([gthread], [gthread-2.0])
	PKG_CHECK_MODULES([liblzma], [liblzma])
fi

ADD_PRIVATE_LIBRARY([crypto])
ADD_PRIVATE_LIBRARY([libutil])
ADD_PRIVATE_LIBRARY([sqlite])
AC_SUBST([pkgsysconfdir], [\$\(sysconfdir\)/$PACKAGE])
AC_SUBST([pkgstatedir], [\$\(localstatedir\)/lib/$PACKAGE])
AC_CONFIG_FILES([Makefile
                 conf/Makefile
                 client/Makefile
                 crypto/Makefile
                 crypto/test/Makefile
                 libutil/Makefile
                 locksrv/Makefile
                 parcelkeeper/Makefile
                 sqlite/Makefile
                 tools/Makefile
                 vmm/Makefile])
AC_OUTPUT
