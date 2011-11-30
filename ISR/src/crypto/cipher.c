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

#include <stdlib.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

static const struct isrcry_cipher_desc *cipher_desc(enum isrcry_cipher type)
{
	switch (type) {
	case ISRCRY_CIPHER_AES:
		return &_isrcry_aes_desc;
	}
	return NULL;
}

static const struct isrcry_mode_desc *mode_desc(enum isrcry_mode type)
{
	switch (type) {
	case ISRCRY_MODE_ECB:
		return &_isrcry_ecb_desc;
	case ISRCRY_MODE_CBC:
		return &_isrcry_cbc_desc;
	}
	return NULL;
}

static const struct isrcry_pad_desc *pad_desc(enum isrcry_padding type)
{
	switch (type) {
	case ISRCRY_PADDING_PKCS5:
		return &_isrcry_pkcs5_desc;
	}
	return NULL;
}

exported struct isrcry_cipher_ctx *isrcry_cipher_alloc(
			enum isrcry_cipher cipher, enum isrcry_mode mode)
{
	struct isrcry_cipher_ctx *cctx;
	
	cctx = g_slice_new0(struct isrcry_cipher_ctx);
	cctx->cipher = cipher_desc(cipher);
	cctx->mode = mode_desc(mode);
	if (cctx->cipher == NULL || cctx->mode == NULL) {
		g_slice_free(struct isrcry_cipher_ctx, cctx);
		return NULL;
	}
	cctx->key = g_slice_alloc0(cctx->cipher->ctxlen);
	return cctx;
}

exported void isrcry_cipher_free(struct isrcry_cipher_ctx *cctx)
{
	g_slice_free1(cctx->cipher->ctxlen, cctx->key);
	g_slice_free(struct isrcry_cipher_ctx, cctx);
}

exported enum isrcry_result isrcry_cipher_init(struct isrcry_cipher_ctx *cctx,
			enum isrcry_direction direction, const void *key,
			int keylen, const void *iv)
{
	enum isrcry_result ret;
	
	g_assert(MAX_BLOCK_LEN >= cctx->cipher->blocklen);
	ret = cctx->cipher->init(cctx, key, keylen);
	if (ret)
		return ret;
	switch (direction) {
	case ISRCRY_ENCRYPT:
	case ISRCRY_DECRYPT:
		break;
	default:
		return ISRCRY_INVALID_ARGUMENT;
	}
	cctx->direction = direction;
	if (iv != NULL)
		memcpy(cctx->iv, iv, cctx->cipher->blocklen);
	else
		memset(cctx->iv, 0, cctx->cipher->blocklen);
	return ISRCRY_OK;
}

exported enum isrcry_result isrcry_cipher_process(
			struct isrcry_cipher_ctx *cctx, const void *in,
			unsigned inlen, void *out)
{
	if (cctx->direction == ISRCRY_ENCRYPT)
		return cctx->mode->encrypt(cctx, in, inlen, out);
	else
		return cctx->mode->decrypt(cctx, in, inlen, out);
}

exported enum isrcry_result isrcry_cipher_final(
			struct isrcry_cipher_ctx *cctx,
			enum isrcry_padding padding, const void *in,
			unsigned inlen, void *out, unsigned *outlen)
{
	const struct isrcry_pad_desc *desc;
	enum isrcry_result ret;
	unsigned char lblock[MAX_BLOCK_LEN];
	unsigned lblock_offset;
	unsigned lblock_len;
	unsigned blocklen;

	if (cctx == NULL || in == NULL || out == NULL || outlen == NULL)
		return ISRCRY_INVALID_ARGUMENT;
	desc = pad_desc(padding);
	if (desc == NULL)
		return ISRCRY_INVALID_ARGUMENT;
	blocklen = cctx->cipher->blocklen;
	if (cctx->direction == ISRCRY_ENCRYPT) {
		lblock_len = inlen % blocklen;
		lblock_offset = inlen - lblock_len;
		if (*outlen < lblock_offset + blocklen)
			return ISRCRY_INVALID_ARGUMENT;
		memcpy(lblock, in + lblock_offset, lblock_len);
		ret = desc->pad(lblock, blocklen, lblock_len);
		if (ret)
			return ret;
		ret = cctx->mode->encrypt(cctx, in, lblock_offset, out);
		if (ret)
			return ret;
		ret = cctx->mode->encrypt(cctx, lblock, blocklen,
					out + lblock_offset);
		if (ret)
			return ret;
		*outlen = lblock_offset + blocklen;
	} else {
		if (inlen == 0 || inlen % blocklen)
			return ISRCRY_INVALID_ARGUMENT;
		lblock_offset = inlen - blocklen;
		if (*outlen < lblock_offset)
			return ISRCRY_INVALID_ARGUMENT;
		ret = cctx->mode->decrypt(cctx, in, lblock_offset, out);
		if (ret)
			return ret;
		ret = cctx->mode->decrypt(cctx, in + lblock_offset, blocklen,
					lblock);
		if (ret)
			return ret;
		ret = desc->unpad(lblock, blocklen, &lblock_len);
		if (ret)
			return ret;
		if (*outlen < lblock_offset + lblock_len)
			return ISRCRY_INVALID_ARGUMENT;
		memcpy(out + lblock_offset, lblock, lblock_len);
		*outlen = lblock_offset + lblock_len;
	}
	return ISRCRY_OK;
}

exported unsigned isrcry_cipher_block(enum isrcry_cipher type)
{
	const struct isrcry_cipher_desc *desc = cipher_desc(type);
	if (desc == NULL)
		return 0;
	return desc->blocklen;
}
