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

/* HMAC, as defined by RFC 2104. */

#define IPAD 0x36363636
#define OPAD 0x5C5C5C5C

struct isrcry_hmac_ctx {
	struct isrcry_hash_ctx *hctx;
	char *key;
	unsigned keylen;
	unsigned hashlen;
};

static void *hmac_alloc(struct isrcry_mac_ctx *mctx)
{
	struct isrcry_hmac_ctx *ctx;

	ctx = g_slice_new0(struct isrcry_hmac_ctx);
	ctx->hctx = isrcry_hash_alloc(mctx->desc->hash);
	if (ctx->hctx == NULL) {
		g_slice_free(struct isrcry_hmac_ctx, ctx);
		return NULL;
	}
	ctx->hashlen = isrcry_hash_len(mctx->desc->hash);
	ctx->keylen = ctx->hctx->desc->block_size;
	ctx->key = g_slice_alloc0(ctx->keylen);
	return ctx;
}

static enum isrcry_result hmac_init(struct isrcry_mac_ctx *mctx,
			const unsigned char *key, unsigned keylen)
{
	struct isrcry_hmac_ctx *ctx = mctx->ctx;
	unsigned char kbuf[ctx->keylen];
	unsigned char hbuf[ctx->hashlen];
	unsigned n;

	/* Make sure the hash is reset */
	isrcry_hash_final(ctx->hctx, hbuf);
	if (keylen > ctx->keylen) {
		isrcry_hash_update(ctx->hctx, key, keylen);
		isrcry_hash_final(ctx->hctx, ctx->key);
	} else {
		memcpy(ctx->key, key, keylen);
		memset(ctx->key + keylen, 0, ctx->keylen - keylen);
	}
	g_assert(ctx->keylen % 4 == 0);
	for (n = 0; n < ctx->keylen / 4; n++)
		((uint32_t *) kbuf)[n] = ((uint32_t *) ctx->key)[n] ^ IPAD;
	isrcry_hash_update(ctx->hctx, kbuf, sizeof(kbuf));
	return ISRCRY_OK;
}

static void hmac_update(struct isrcry_mac_ctx *mctx,
			const unsigned char *buffer, unsigned length)
{
	struct isrcry_hmac_ctx *ctx = mctx->ctx;

	isrcry_hash_update(ctx->hctx, buffer, length);
}

static enum isrcry_result hmac_final(struct isrcry_mac_ctx *mctx,
			unsigned char *out, unsigned outlen)
{
	struct isrcry_hmac_ctx *ctx = mctx->ctx;
	unsigned char kbuf[ctx->keylen];
	unsigned char hbuf[ctx->hashlen];
	unsigned n;

	if (outlen > ctx->hashlen)
		return ISRCRY_INVALID_ARGUMENT;
	isrcry_hash_final(ctx->hctx, hbuf);
	g_assert(ctx->keylen % 4 == 0);
	for (n = 0; n < ctx->keylen / 4; n++)
		((uint32_t *) kbuf)[n] = ((uint32_t *) ctx->key)[n] ^ OPAD;
	isrcry_hash_update(ctx->hctx, kbuf, sizeof(kbuf));
	isrcry_hash_update(ctx->hctx, hbuf, sizeof(hbuf));
	isrcry_hash_final(ctx->hctx, hbuf);
	memcpy(out, hbuf, outlen);
	return ISRCRY_OK;
}

static void hmac_free(struct isrcry_mac_ctx *mctx)
{
	struct isrcry_hmac_ctx *ctx = mctx->ctx;

	g_slice_free1(ctx->keylen, ctx->key);
	isrcry_hash_free(ctx->hctx);
	g_slice_free(struct isrcry_hmac_ctx, ctx);
}

const struct isrcry_mac_desc _isrcry_hmac_sha1_desc = {
	.alloc = hmac_alloc,
	.init = hmac_init,
	.update = hmac_update,
	.final = hmac_final,
	.free = hmac_free,
	.hash = ISRCRY_HASH_SHA1,
	.mac_size = 20
};
