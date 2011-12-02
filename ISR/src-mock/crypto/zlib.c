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

#include <zlib.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

static enum isrcry_result zlib_error(int err)
{
	switch (err) {
	case Z_STREAM_ERROR:
		return ISRCRY_INVALID_ARGUMENT;
	case Z_NEED_DICT:
	case Z_DATA_ERROR:
		return ISRCRY_BAD_FORMAT;
	case Z_BUF_ERROR:
		return ISRCRY_BUFFER_OVERFLOW;
	case Z_ERRNO:
	case Z_MEM_ERROR:
	case Z_VERSION_ERROR:
		g_assert_not_reached();
	default:
		return ISRCRY_OK;
	}
}

static void stream_setup(z_stream *strm, const unsigned char *in,
			unsigned *inlen, unsigned char *out, unsigned *outlen)
{
	strm->next_in = (unsigned char *) in;
	strm->avail_in = *inlen;
	strm->next_out = out;
	strm->avail_out = *outlen;
}

static void stream_result(z_stream *strm, unsigned *inlen, unsigned *outlen)
{
	*inlen -= strm->avail_in;
	*outlen -= strm->avail_out;
}

static enum isrcry_result zlib_alloc(struct isrcry_compress_ctx *cctx)
{
	z_stream *strm;
	int ret;

	if (cctx->level == 0)
		cctx->level = Z_DEFAULT_COMPRESSION;
	strm = g_slice_new0(z_stream);
	if (cctx->direction == ISRCRY_ENCODE) {
		ret = deflateInit(strm, cctx->level);
		if (ret)
			deflateEnd(strm);
	} else {
		ret = inflateInit(strm);
		if (ret)
			inflateEnd(strm);
	}
	if (ret) {
		g_slice_free(z_stream, strm);
		return zlib_error(ret);
	}
	cctx->ctx = strm;
	return ISRCRY_OK;
}

static void zlib_free(struct isrcry_compress_ctx *cctx)
{
	z_stream *strm = cctx->ctx;

	if (cctx->direction == ISRCRY_ENCODE)
		deflateEnd(strm);
	else
		inflateEnd(strm);
	g_slice_free(z_stream, strm);
}

static enum isrcry_result zlib_compress_process(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	z_stream *strm = cctx->ctx;
	int ret;

	stream_setup(strm, in, inlen, out, outlen);
	ret = deflate(strm, 0);
	stream_result(strm, inlen, outlen);
	return zlib_error(ret);
}

static enum isrcry_result zlib_compress_final(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	z_stream *strm = cctx->ctx;
	int ret;

	stream_setup(strm, in, inlen, out, outlen);
	ret = deflate(strm, Z_FINISH);
	stream_result(strm, inlen, outlen);
	if (ret == Z_OK)
		return ISRCRY_BUFFER_OVERFLOW;
	return zlib_error(ret);
}

static enum isrcry_result zlib_decompress_process(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	z_stream *strm = cctx->ctx;
	int ret;

	stream_setup(strm, in, inlen, out, outlen);
	ret = inflate(strm, 0);
	stream_result(strm, inlen, outlen);
	if (strm->avail_in > 0 && ret == Z_STREAM_END)
		return ISRCRY_BAD_FORMAT;
	return zlib_error(ret);
}

static enum isrcry_result zlib_decompress_final(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	z_stream *strm = cctx->ctx;
	int ret;

	stream_setup(strm, in, inlen, out, outlen);
	ret = inflate(strm, Z_FINISH);
	stream_result(strm, inlen, outlen);
	if (strm->avail_in > 0 && ret == Z_STREAM_END)
		return ISRCRY_BAD_FORMAT;
	if (ret == Z_OK)
		return ISRCRY_BUFFER_OVERFLOW;
	return zlib_error(ret);
}

const struct isrcry_compress_desc _isrcry_zlib_desc = {
	.can_stream = TRUE,
	.alloc = zlib_alloc,
	.free = zlib_free,
	.compress_process = zlib_compress_process,
	.compress_final = zlib_compress_final,
	.decompress_process = zlib_decompress_process,
	.decompress_final = zlib_decompress_final
};
