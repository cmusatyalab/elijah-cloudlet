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

static const struct isrcry_hash_desc *hash_desc(enum isrcry_hash type)
{
	switch (type) {
	case ISRCRY_HASH_SHA1:
		return &_isrcry_sha1_desc;
	case ISRCRY_HASH_MD5:
		return &_isrcry_md5_desc;
	}
	return NULL;
}

exported struct isrcry_hash_ctx *isrcry_hash_alloc(enum isrcry_hash type)
{
	struct isrcry_hash_ctx *hctx;
	
	hctx = g_slice_new0(struct isrcry_hash_ctx);
	hctx->desc = hash_desc(type);
	if (hctx->desc == NULL) {
		g_slice_free(struct isrcry_hash_ctx, hctx);
		return NULL;
	}
	hctx->ctx = g_slice_alloc0(hctx->desc->ctxlen);
	hctx->desc->init(hctx);
	return hctx;
}

exported void isrcry_hash_free(struct isrcry_hash_ctx *hctx)
{
	g_slice_free1(hctx->desc->ctxlen, hctx->ctx);
	g_slice_free(struct isrcry_hash_ctx, hctx);
}

exported void isrcry_hash_update(struct isrcry_hash_ctx *hctx,
			const void *buffer, unsigned length)
{
	hctx->desc->update(hctx, buffer, length);
}

exported void isrcry_hash_final(struct isrcry_hash_ctx *hctx, void *digest)
{
	hctx->desc->final(hctx, digest);
	hctx->desc->init(hctx);
}

exported unsigned isrcry_hash_len(enum isrcry_hash type)
{
	const struct isrcry_hash_desc *desc = hash_desc(type);
	if (desc == NULL)
		return 0;
	return desc->digest_size;
}
