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

#define MEMLIMIT_DECODE (1ULL << 30)

#include <lzma.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

static enum isrcry_result lzma_error(lzma_ret err)
{
	switch (err) {
	case LZMA_OK:
	case LZMA_STREAM_END:
		return ISRCRY_OK;
	case LZMA_UNSUPPORTED_CHECK:
	case LZMA_OPTIONS_ERROR:
	case LZMA_PROG_ERROR:
		return ISRCRY_INVALID_ARGUMENT;
	case LZMA_FORMAT_ERROR:
	case LZMA_DATA_ERROR:
		return ISRCRY_BAD_FORMAT;
	case LZMA_BUF_ERROR:
		return ISRCRY_BUFFER_OVERFLOW;
	default:
		g_assert_not_reached();
	}
}

static enum isrcry_result lzma_alloc(struct isrcry_compress_ctx *cctx)
{
	lzma_stream *strm;
	lzma_ret ret;

	if (cctx->level == 0)
		cctx->level = 7;  /* Higher than the liblzma default */
	strm = g_slice_new0(lzma_stream);
	if (cctx->direction == ISRCRY_ENCODE)
		ret = lzma_easy_encoder(strm, cctx->level, LZMA_CHECK_CRC32);
	else
		ret = lzma_stream_decoder(strm, MEMLIMIT_DECODE, 0);
	if (ret) {
		g_slice_free(lzma_stream, strm);
		return lzma_error(ret);
	}
	cctx->ctx = strm;
	return ISRCRY_OK;
}

static void lzma_free(struct isrcry_compress_ctx *cctx)
{
	lzma_stream *strm = cctx->ctx;

	lzma_end(strm);
	g_slice_free(lzma_stream, strm);
}

static enum isrcry_result lzma_run(struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen,
				gboolean final)
{
	lzma_stream *strm = cctx->ctx;
	lzma_ret ret;

	strm->next_in = in;
	strm->avail_in = *inlen;
	strm->next_out = out;
	strm->avail_out = *outlen;
	ret = lzma_code(strm, final ? LZMA_FINISH : LZMA_RUN);
	*inlen -= strm->avail_in;
	*outlen -= strm->avail_out;

	/* If finalizing, check for remaining work */
	if (final && ret == LZMA_OK)
		return ISRCRY_BUFFER_OVERFLOW;
	/* On decode, check for trailing garbage */
	if (cctx->direction == ISRCRY_DECODE &&
				strm->avail_in > 0 && ret == LZMA_STREAM_END)
		return ISRCRY_BAD_FORMAT;
	return lzma_error(ret);
}

static enum isrcry_result lzma_process(struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	return lzma_run(cctx, in, inlen, out, outlen, FALSE);
}

static enum isrcry_result lzma_final(struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	return lzma_run(cctx, in, inlen, out, outlen, TRUE);
}

const struct isrcry_compress_desc _isrcry_lzma_desc = {
	.can_stream = TRUE,
	.alloc = lzma_alloc,
	.free = lzma_free,
	.compress_process = lzma_process,
	.compress_final = lzma_final,
	.decompress_process = lzma_process,
	.decompress_final = lzma_final
};
