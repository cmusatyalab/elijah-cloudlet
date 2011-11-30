/*
 * libisrutil - utility library for the OpenISR (R) system
 *
 * Copyright (C) 2010 Carnegie Mellon University
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef LIBISRUTIL_INTERNAL_H
#define LIBISRUTIL_INTERNAL_H

#ifndef LIBISRUTIL_INTERNAL
#error This header is for internal use by libisrutil
#endif

#define G_LOG_DOMAIN "isrutil"

#include "isrutil.h"
#include "config.h"

#ifdef HAVE_VISIBILITY
#define exported __attribute__ ((visibility ("default")))
#else
#define exported
#endif

#endif
