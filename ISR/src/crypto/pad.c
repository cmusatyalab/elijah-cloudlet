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

#include <unistd.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

static enum isrcry_result pkcs5_pad(unsigned char *buf, unsigned blocklen,
			unsigned datalen)
{
	unsigned char pad;
	unsigned n;

	if (buf == NULL || datalen >= blocklen || blocklen - datalen > 255)
		return ISRCRY_INVALID_ARGUMENT;
	pad = blocklen - datalen;
	for (n = datalen; n < blocklen; n++)
		buf[n] = pad;
	return ISRCRY_OK;
}

static enum isrcry_result pkcs5_unpad(unsigned char *buf, unsigned blocklen,
			unsigned *datalen)
{
	unsigned char pad;
	unsigned n;

	if (buf == NULL || datalen == NULL)
		return ISRCRY_INVALID_ARGUMENT;
	pad = buf[blocklen - 1];
	if (pad == 0 || pad > blocklen)
		return ISRCRY_BAD_PADDING;
	for (n = 1; n < pad; n++)
		if (buf[blocklen - n - 1] != pad)
			return ISRCRY_BAD_PADDING;
	*datalen = blocklen - pad;
	return ISRCRY_OK;
}

const struct isrcry_pad_desc _isrcry_pkcs5_desc = {
	.pad = pkcs5_pad,
	.unpad = pkcs5_unpad
};
