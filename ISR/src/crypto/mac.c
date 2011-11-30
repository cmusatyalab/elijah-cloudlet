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

static const struct isrcry_mac_desc *mac_desc(enum isrcry_mac type)
{
	switch (type) {
	case ISRCRY_MAC_HMAC_SHA1:
		return &_isrcry_hmac_sha1_desc;
	}
	return NULL;
}

exported struct isrcry_mac_ctx *isrcry_mac_alloc(enum isrcry_mac type)
{
	struct isrcry_mac_ctx *mctx;

	mctx = g_slice_new0(struct isrcry_mac_ctx);
	mctx->desc = mac_desc(type);
	if (mctx->desc == NULL) {
		g_slice_free(struct isrcry_mac_ctx, mctx);
		return NULL;
	}
	mctx->ctx = mctx->desc->alloc(mctx);
	if (mctx->ctx == NULL) {
		g_slice_free(struct isrcry_mac_ctx, mctx);
		return NULL;
	}
	return mctx;
}

exported void isrcry_mac_free(struct isrcry_mac_ctx *mctx)
{
	mctx->desc->free(mctx);
	g_free(mctx->key);
	g_slice_free(struct isrcry_mac_ctx, mctx);
}

static enum isrcry_result check_init(struct isrcry_mac_ctx *mctx)
{
	enum isrcry_result ret;

	if (!mctx->inited) {
		ret = mctx->desc->init(mctx, mctx->key, mctx->keylen);
		if (ret)
			return ret;
		mctx->inited = TRUE;
	}
	return ISRCRY_OK;
}

exported enum isrcry_result isrcry_mac_init(struct isrcry_mac_ctx *mctx,
			const void *key, unsigned keylen)
{
	if (mctx->key != NULL)
		g_free(mctx->key);
	mctx->key = g_memdup(key, keylen);
	mctx->keylen = keylen;
	mctx->inited = FALSE;
	/* Key the MAC immediately, so that if the key is invalid, we get
	   an error here and not in isrcry_mac_update() */
	return check_init(mctx);
}

exported enum isrcry_result isrcry_mac_update(struct isrcry_mac_ctx *mctx,
			const void *buffer, unsigned length)
{
	enum isrcry_result ret;

	ret = check_init(mctx);
	if (ret)
		return ret;
	mctx->desc->update(mctx, buffer, length);
	return ISRCRY_OK;
}

exported enum isrcry_result isrcry_mac_final(struct isrcry_mac_ctx *mctx,
			void *out, unsigned outlen)
{
	enum isrcry_result ret;

	ret = check_init(mctx);
	if (ret)
		return ret;
	ret = mctx->desc->final(mctx, out, outlen);
	mctx->inited = FALSE;
	return ret;
}

exported unsigned isrcry_mac_len(enum isrcry_mac type)
{
	const struct isrcry_mac_desc *desc = mac_desc(type);
	if (desc == NULL)
		return 0;
	return desc->mac_size;
}
