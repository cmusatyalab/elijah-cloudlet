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

static const struct isrcry_compress_desc *compress_desc(
			enum isrcry_compress type)
{
	switch (type) {
	case ISRCRY_COMPRESS_ZLIB:
		return &_isrcry_zlib_desc;
	case ISRCRY_COMPRESS_LZF:
		return &_isrcry_lzf_desc;
	case ISRCRY_COMPRESS_LZF_STREAM:
		return &_isrcry_lzf_stream_desc;
	case ISRCRY_COMPRESS_LZMA:
		return &_isrcry_lzma_desc;
	}
	return NULL;
}

exported struct isrcry_compress_ctx *isrcry_compress_alloc(
			enum isrcry_compress compress)
{
	struct isrcry_compress_ctx *cctx;

	cctx = g_slice_new0(struct isrcry_compress_ctx);
	cctx->desc = compress_desc(compress);
	if (cctx->desc == NULL) {
		g_slice_free(struct isrcry_compress_ctx, cctx);
		return NULL;
	}
	return cctx;
}

exported enum isrcry_result isrcry_compress_init(
			struct isrcry_compress_ctx *cctx,
			enum isrcry_direction direction, int level)
{
	switch (direction) {
	case ISRCRY_ENCODE:
	case ISRCRY_DECODE:
		break;
	default:
		return ISRCRY_INVALID_ARGUMENT;
	}
	if (cctx->ctx != NULL) {
		cctx->desc->free(cctx);
		cctx->ctx = NULL;
	}
	cctx->direction = direction;
	cctx->level = level;
	return cctx->desc->alloc(cctx);
}

exported void isrcry_compress_free(struct isrcry_compress_ctx *cctx)
{
	if (cctx->ctx != NULL)
		cctx->desc->free(cctx);
	g_slice_free(struct isrcry_compress_ctx, cctx);
}

exported enum isrcry_result isrcry_compress_process(
			struct isrcry_compress_ctx *cctx, const void *in,
			unsigned *inlen, void *out, unsigned *outlen)
{
	if (!cctx->desc->can_stream) {
		*inlen = *outlen = 0;
		return ISRCRY_NO_STREAMING;
	}
	if (cctx->direction == ISRCRY_ENCODE)
		return cctx->desc->compress_process(cctx, in, inlen, out,
					outlen);
	else
		return cctx->desc->decompress_process(cctx, in, inlen, out,
					outlen);
}

exported enum isrcry_result isrcry_compress_final(
			struct isrcry_compress_ctx *cctx, const void *in,
			unsigned *inlen, void *out, unsigned *outlen)
{
	if (cctx->direction == ISRCRY_ENCODE)
		return cctx->desc->compress_final(cctx, in, inlen, out,
					outlen);
	else
		return cctx->desc->decompress_final(cctx, in, inlen, out,
					outlen);
}

exported int isrcry_compress_can_stream(enum isrcry_compress compress)
{
	const struct isrcry_compress_desc *desc = compress_desc(compress);

	if (desc == NULL)
		return FALSE;
	return desc->can_stream;
}
