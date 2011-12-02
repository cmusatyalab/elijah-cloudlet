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

/*
 * We use the same format as the "lzf" command-line tool shipped with
 * liblzf.  From lzf.c:
 *
 *   An lzf file consists of any number of blocks in the following format:
 *
 *   \x00   EOF (optional)
 *   "ZV\0" 2-byte-usize <uncompressed data>
 *   "ZV\1" 2-byte-csize 2-byte-usize <compressed data>
 *   "ZV\2" 4-byte-crc32-0xdebb20e3 (NYI)
 *
 * (0xdebb20e3 turns out to be an odd way of spelling the IEEE 802.3 CRC used
 * by zlib.)
 *
 * According to Marc Lehmann, ZV2 will most likely never be implemented
 * upstream.  If it were, he says it would likely be a running CRC of the ZV0
 * and ZV1 payloads (i.e., the compressed data).  However, we want a checksum
 * of the *uncompressed* data to catch bugs in LZF and the buffering code.
 * Therefore, rather than implementing ZV2, we make up our own block:
 *
 *   "ZV\x30" 4-byte-big-endian-IEEE-802.3-CRC32-of-uncompressed-data
 *
 * We could use Adler32 instead for a marginal increase in performance, but
 * the net difference in throughput is not large, and CRC32 is both more
 * effective and more widely used.
 */

#define MAX_BLOCK ((1 << 16) - 1)
#define MAX_HEADER 7  /* total header length */
#define BUFLEN (MAX_BLOCK + MAX_HEADER)

#include <zlib.h>  /* for crc32() */
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

enum lzf_decode_state {
	WANT_HEADER_1,
	WANT_HEADER_2,
	WANT_HEADER_3,
	WANT_DATA,
	AT_EOF,
	DATA_ERROR
};

struct lzf_stream_ctx {
	struct isrcry_compress_ctx *block_ctx;
	unsigned crc;

	/* Encode state */
	gboolean wrote_trailer;

	/* Decode state */
	enum lzf_decode_state decode_state;
	int block_type;
	unsigned output_size;

	/* I/O buffering */
	uint8_t *in;
	unsigned in_offset;
	unsigned in_count;  /* bytes we need to process further */
	uint8_t *out;
	unsigned out_offset;
	unsigned out_count;
};

static void set_state(struct lzf_stream_ctx *sctx,
			enum lzf_decode_state state, ...)
{
	va_list ap;

	sctx->decode_state = state;
	va_start(ap, state);
	switch (state) {
	case WANT_HEADER_1:
		sctx->in_count = 1;
		break;
	case WANT_HEADER_2:
		sctx->in_count = 2;
		break;
	case WANT_HEADER_3:
	case WANT_DATA:
		sctx->in_count = va_arg(ap, unsigned);
		break;
	case AT_EOF:
	case DATA_ERROR:
		/* No more input accepted */
		sctx->in_count = 0;
		break;
	}
	va_end(ap);
}

/* Given the specified input buffer and count, attempt to produce
   sctx->in_count bytes of input.  Update *buf and *count according to what
   we consumed.  Returns a pointer to in_count bytes, or NULL if we need more
   bytes.  Tries to return a pointer within *buf if possible, but copies
   to a temporary buffer if it's necessary to accumulate in_count bytes
   across multiple calls. */
static const void *consume_input(struct lzf_stream_ctx *sctx,
			const void **buf, unsigned *count)
{
	const void *ret;
	unsigned consuming;

	if (sctx->in_offset == 0 && *count >= sctx->in_count) {
		ret = *buf;
		*buf += sctx->in_count;
		*count -= sctx->in_count;
		return ret;
	}
	consuming = MIN(*count, sctx->in_count - sctx->in_offset);
	memcpy(sctx->in + sctx->in_offset, *buf, consuming);
	*buf += consuming;
	*count -= consuming;
	sctx->in_offset += consuming;
	if (sctx->in_offset == sctx->in_count) {
		sctx->in_offset = 0;
		return sctx->in;
	}
	return NULL;
}

/* Return TRUE if we're in the process of reading data, FALSE if we're
   at EOF or between compression blocks. */
static gboolean input_data_pending(struct lzf_stream_ctx *sctx)
{
	switch (sctx->decode_state) {
	case WANT_HEADER_1:
	case AT_EOF:
		return sctx->in_offset ? TRUE : FALSE;
	default:
		return TRUE;
	}
}

/* Return TRUE if output data is buffered, FALSE otherwise. */
static gboolean output_data_is_buffered(struct lzf_stream_ctx *sctx)
{
	return sctx->out_count ? TRUE : FALSE;
}

/* Return a pointer to a place to store len bytes of output.  *outbuf and
   *outlen are the available user-supplied output buffer and length, and
   are updated if we decide to use them.  We return *outbuf if it has
   enough room for len bytes, or a pointer to a temporary buffer if *outbuf
   is too small.  Returns NULL if we can't store this much data right now
   (because the temporary buffer is already in use). */
static void *get_output_buffer(struct lzf_stream_ctx *sctx, unsigned len,
			void **outbuf, unsigned *outlen)
{
	void *ret;

	/* If there's data in the output buffer, it must be flushed before
	   we can do anything else */
	if (output_data_is_buffered(sctx))
		return NULL;
	if (*outlen >= len) {
		ret = *outbuf;
		*outbuf += len;
		*outlen -= len;
		return ret;
	}
	sctx->out_count = len;
	return sctx->out;
}

/* Copy up to *count bytes of output from the output buffer into *buf, and
   update *buf and *count according to the amount of space we used. */
static void produce_output(struct lzf_stream_ctx *sctx, void **buf,
			unsigned *count)
{
	unsigned producing;

	producing = MIN(*count, sctx->out_count - sctx->out_offset);
	memcpy(*buf, sctx->out + sctx->out_offset, producing);
	*buf += producing;
	*count -= producing;
	sctx->out_offset += producing;
	if (sctx->out_offset == sctx->out_count)
		sctx->out_offset = sctx->out_count = 0;
}

static enum isrcry_result lzf_stream_alloc(struct isrcry_compress_ctx *cctx)
{
	struct lzf_stream_ctx *sctx;
	struct isrcry_compress_ctx *block_ctx;
	enum isrcry_result ret;

	block_ctx = isrcry_compress_alloc(ISRCRY_COMPRESS_LZF);
	if (block_ctx == NULL)
		return ISRCRY_INVALID_ARGUMENT;
	ret = isrcry_compress_init(block_ctx, cctx->direction, cctx->level);
	if (ret) {
		isrcry_compress_free(block_ctx);
		return ret;
	}
	sctx = g_slice_new0(struct lzf_stream_ctx);
	sctx->block_ctx = block_ctx;
	sctx->crc = crc32(0, NULL, 0);
	set_state(sctx, WANT_HEADER_1);
	sctx->in = g_slice_alloc(BUFLEN);
	sctx->out = g_slice_alloc(BUFLEN);
	cctx->ctx = sctx;
	return ISRCRY_OK;
}

static void lzf_stream_free(struct isrcry_compress_ctx *cctx)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;

	isrcry_compress_free(sctx->block_ctx);
	g_slice_free1(BUFLEN, sctx->in);
	g_slice_free1(BUFLEN, sctx->out);
	g_slice_free(struct lzf_stream_ctx, sctx);
}

/* Encode inlen bytes from in, into *outbuf (up to *outlen bytes) or into
   the output buffer */
static void data_encode(struct isrcry_compress_ctx *cctx, const void *in,
			unsigned inlen, void **out, unsigned *outlen)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	uint8_t *outbuf;
	unsigned in_count = inlen;
	unsigned out_count = MAX_BLOCK;
	enum isrcry_result ret;

	/* We don't know the output length until we try, so directly
	   write to the output buffer rather than fighting with the
	   requirements of get_output_buffer().  Then try pushing into
	   *outbuf afterward. */
	sctx->crc = crc32(sctx->crc, in, inlen);
	ret = isrcry_compress_final(sctx->block_ctx, in, &in_count,
				sctx->out + 7, &out_count);
	isrcry_compress_init(sctx->block_ctx, cctx->direction, cctx->level);
	if (ret) {
		/* Store uncompressed.  Since we know the length we can use
		   get_output_buffer() for this. */
		outbuf = get_output_buffer(sctx, inlen + 5, out, outlen);
		memcpy(outbuf, "ZV\0", 3);
		STORE16H(inlen, &outbuf[3]);
		memcpy(outbuf + 5, in, inlen);
	} else {
		memcpy(sctx->out, "ZV\1", 3);
		STORE16H(out_count, &sctx->out[3]);
		STORE16H(in_count, &sctx->out[5]);
		sctx->out_count = out_count + 7;
		produce_output(sctx, out, outlen);
	}
}

static enum isrcry_result lzf_stream_compress_process(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	const void *inp = in;
	void *outp = out;
	unsigned in_avail = *inlen;
	unsigned out_avail = *outlen;
	const void *inbuf;

	produce_output(sctx, &outp, &out_avail);
	while (1) {
		/* We can only make further progress if we can use the output
		   buffer, which requires that there's no data already
		   there */
		if (output_data_is_buffered(sctx))
			break;
		sctx->in_count = MAX_BLOCK;
		inbuf = consume_input(sctx, &inp, &in_avail);
		if (inbuf == NULL)
			break;
		data_encode(cctx, inbuf, sctx->in_count, &outp, &out_avail);
	}

	*inlen -= in_avail;
	*outlen -= out_avail;
	return ISRCRY_OK;
}

static enum isrcry_result lzf_stream_compress_final(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	unsigned outlen_orig = *outlen;
	void *outp;
	unsigned out_avail;
	uint8_t *outbuf;
	enum isrcry_result ret;

	ret = lzf_stream_compress_process(cctx, in, inlen, out, outlen);
	if (ret)
		return ret;
	/* See if there's remaining output data. */
	if (output_data_is_buffered(sctx))
		return ISRCRY_BUFFER_OVERFLOW;
	/* If not, we've already buffered or processed all remaining input
	   data.  Process buffered input data. */
	outp = out + *outlen;
	out_avail = outlen_orig - *outlen;
	if (input_data_pending(sctx)) {
		data_encode(cctx, sctx->in, sctx->in_offset, &outp,
					&out_avail);
		sctx->in_offset = 0;
		*outlen = outlen_orig - out_avail;
	}
	/* Make sure we had enough space for that output. */
	if (output_data_is_buffered(sctx))
		return ISRCRY_BUFFER_OVERFLOW;
	/* Write the trailer. */
	if (!sctx->wrote_trailer) {
		outbuf = get_output_buffer(sctx, 7, &outp, &out_avail);
		memcpy(outbuf, "ZV\x30", 3);
		STORE32H(sctx->crc, &outbuf[3]);
		sctx->wrote_trailer = TRUE;
		*outlen = outlen_orig - out_avail;
	}
	/* Make sure we had enough space for that output. */
	if (output_data_is_buffered(sctx))
		return ISRCRY_BUFFER_OVERFLOW;
	return ISRCRY_OK;
}

/* Process first character in block header, since NUL is a valid header
   by itself */
static void header_decode_1(struct lzf_stream_ctx *sctx, const uint8_t *buf)
{
	switch (*buf) {
	case 0:
		set_state(sctx, AT_EOF);
		break;
	case 'Z':
		set_state(sctx, WANT_HEADER_2);
		break;
	default:
		set_state(sctx, DATA_ERROR);
		break;
	}
}

/* Process remaining fixed-length part of block header, until we know what
   block type this is */
static void header_decode_2(struct lzf_stream_ctx *sctx, const uint8_t *buf)
{
	if (buf[0] != 'V') {
		set_state(sctx, DATA_ERROR);
		return;
	}
	sctx->block_type = buf[1];
	switch (sctx->block_type) {
	case 0:
		set_state(sctx, WANT_HEADER_3, 2);
		break;
	case 1:
		set_state(sctx, WANT_HEADER_3, 4);
		break;
	case 0x30:
		set_state(sctx, WANT_HEADER_3, 4);
		break;
	default:
		set_state(sctx, DATA_ERROR);
		break;
	}
}

/* Process variable-length part of block header */
static void header_decode_3(struct lzf_stream_ctx *sctx, const uint8_t *buf)
{
	uint16_t csize;
	uint16_t usize;
	uint32_t crc;

	switch (sctx->block_type) {
	case 0:
		/* 2-byte-usize */
		LOAD16H(usize, &buf[0]);
		csize = usize;
		break;
	case 1:
		/* 2-byte-csize 2-byte-usize */
		LOAD16H(csize, &buf[0]);
		LOAD16H(usize, &buf[2]);
		break;
	case 0x30:
		/* IEEE 802.3 CRC of uncompressed data in big-endian order */
		LOAD32H(crc, &buf[0]);
		if (sctx->crc != crc)
			set_state(sctx, DATA_ERROR);
		else
			set_state(sctx, WANT_HEADER_1);
		return;
	default:
		g_assert_not_reached();
	}
	set_state(sctx, WANT_DATA, (unsigned) csize);
	sctx->output_size = usize;
}

/* Decode an LZF data block */
static void data_decode(struct isrcry_compress_ctx *cctx, const void *in,
			void *out)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	enum isrcry_result ret;
	unsigned inlen = sctx->in_count;
	unsigned outlen = sctx->output_size;

	switch (sctx->block_type) {
	case 0:
		/* Uncompressed */
		g_assert(inlen == outlen);
		memcpy(out, in, inlen);
		break;
	case 1:
		/* Compressed */
		ret = isrcry_compress_final(sctx->block_ctx, in, &inlen, out,
					&outlen);
		isrcry_compress_init(sctx->block_ctx, cctx->direction,
					cctx->level);
		/* ret might be BAD_FORMAT or BUFFER_OVERFLOW.  We map both
		   of these to BAD_FORMAT, since the needed buffer length
		   should be exactly what the block header told us. */
		if (ret || inlen != sctx->in_count ||
					outlen != sctx->output_size) {
			set_state(sctx, DATA_ERROR);
			return;
		}
		break;
	default:
		g_assert_not_reached();
	}
	sctx->crc = crc32(sctx->crc, out, outlen);
	set_state(sctx, WANT_HEADER_1);
}

static enum isrcry_result lzf_stream_decompress_process(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	const void *inp = in;
	void *outp = out;
	unsigned in_avail = *inlen;
	unsigned out_avail = *outlen;
	const void *inbuf;
	void *outbuf;
	enum isrcry_result ret = ISRCRY_OK;

	produce_output(sctx, &outp, &out_avail);
	while (1) {
		/* Don't populate inbuf if we need an outbuf and can't have
		   one */
		if (sctx->decode_state == WANT_DATA &&
					output_data_is_buffered(sctx))
			break;
		inbuf = consume_input(sctx, &inp, &in_avail);
		if (inbuf == NULL)
			break;

		switch (sctx->decode_state) {
		case WANT_HEADER_1:
			header_decode_1(sctx, inbuf);
			break;
		case WANT_HEADER_2:
			header_decode_2(sctx, inbuf);
			break;
		case WANT_HEADER_3:
			header_decode_3(sctx, inbuf);
			break;
		case WANT_DATA:
			outbuf = get_output_buffer(sctx, sctx->output_size,
						&outp, &out_avail);
			data_decode(cctx, inbuf, outbuf);
			break;
		case AT_EOF:
			if (in_avail > 0)
				ret = ISRCRY_BAD_FORMAT;
			goto out;
		case DATA_ERROR:
			ret = ISRCRY_BAD_FORMAT;
			goto out;
		}
	}
out:
	*inlen -= in_avail;
	*outlen -= out_avail;
	return ret;
}

static enum isrcry_result lzf_stream_decompress_final(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen)
{
	struct lzf_stream_ctx *sctx = cctx->ctx;
	enum isrcry_result ret;
	unsigned in_avail = *inlen;

	ret = lzf_stream_decompress_process(cctx, in, inlen, out, outlen);
	if (ret)
		return ret;
	/* If we've consumed all input, check for trailing garbage */
	if (*inlen == in_avail && input_data_pending(sctx))
		return ISRCRY_BAD_FORMAT;
	/* See if there's any remaining output */
	if (output_data_is_buffered(sctx))
		return ISRCRY_BUFFER_OVERFLOW;
	return ISRCRY_OK;
}

const struct isrcry_compress_desc _isrcry_lzf_stream_desc = {
	.can_stream = TRUE,
	.alloc = lzf_stream_alloc,
	.free = lzf_stream_free,
	.compress_process = lzf_stream_compress_process,
	.compress_final = lzf_stream_compress_final,
	.decompress_process = lzf_stream_decompress_process,
	.decompress_final = lzf_stream_decompress_final
};
