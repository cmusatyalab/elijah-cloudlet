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

static enum isrcry_result ecb_encrypt(struct isrcry_cipher_ctx *cctx,
			const unsigned char *in, unsigned len,
			unsigned char *out)
{
	enum isrcry_result ret;
	unsigned blocklen = cctx->cipher->blocklen;

	if (in == NULL || out == NULL || len % blocklen)
		return ISRCRY_INVALID_ARGUMENT;

	while (len) {
		ret = cctx->cipher->encrypt(cctx, in, out);
		if (ret)
			return ret;
		len -= blocklen;
		in += blocklen;
		out += blocklen;
	}
	return ISRCRY_OK;
}

static enum isrcry_result ecb_decrypt(struct isrcry_cipher_ctx *cctx,
			const unsigned char *in, unsigned len,
			unsigned char *out)
{
	enum isrcry_result ret;
	unsigned blocklen = cctx->cipher->blocklen;

	if (in == NULL || out == NULL || len % blocklen)
		return ISRCRY_INVALID_ARGUMENT;

	while (len) {
		ret = cctx->cipher->decrypt(cctx, in, out);
		if (ret)
			return ret;
		len -= blocklen;
		in += blocklen;
		out += blocklen;
	}
	return ISRCRY_OK;
}

const struct isrcry_mode_desc _isrcry_ecb_desc = {
	.encrypt = ecb_encrypt,
	.decrypt = ecb_decrypt
};
