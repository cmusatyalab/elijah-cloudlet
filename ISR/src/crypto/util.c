/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * Copyright (C) 2008-2009 Carnegie Mellon University
 * 
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License as
 * published by the Free Software Foundation.  A copy of the GNU Lesser General
 * Public License should have been distributed along with this library in the
 * file LICENSE.LGPL.
 *          
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 */

#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

exported const char *isrcry_strerror(enum isrcry_result result)
{
	switch (result) {
	case ISRCRY_OK:
		return "Success";
	case ISRCRY_INVALID_ARGUMENT:
		return "Invalid argument";
	case ISRCRY_BAD_PADDING:
		return "Bad padding";
	case ISRCRY_BAD_FORMAT:
		return "Invalid data format";
	case ISRCRY_BUFFER_OVERFLOW:
		return "Buffer too short";
	case ISRCRY_NO_STREAMING:
		return "Algorithm does not support streaming";
	}
	return "Unknown error";
}
