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

#ifndef LIBISRCRYPTO_DEFS_H
#define LIBISRCRYPTO_DEFS_H

#ifndef LIBISRCRYPTO_INTERNAL
#error This header is for internal use by libisrcrypto
#endif

#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include "config.h"

#ifdef HAVE_VISIBILITY
#define exported __attribute__ ((visibility ("default")))
#else
#define exported
#endif

#define MAX_BLOCK_LEN 16

struct isrcry_cipher_desc {
	enum isrcry_result (*init)(struct isrcry_cipher_ctx *cctx,
				const unsigned char *key, int keylen);
	enum isrcry_result (*encrypt)(struct isrcry_cipher_ctx *cctx,
				const unsigned char *in,
				unsigned char *out);
	enum isrcry_result (*decrypt)(struct isrcry_cipher_ctx *cctx,
				const unsigned char *in,
				unsigned char *out);
	unsigned blocklen;
	unsigned ctxlen;
};

extern const struct isrcry_cipher_desc _isrcry_aes_desc;

struct isrcry_mode_desc {
	enum isrcry_result (*encrypt)(struct isrcry_cipher_ctx *cctx,
				const unsigned char *in, unsigned len,
				unsigned char *out);
	enum isrcry_result (*decrypt)(struct isrcry_cipher_ctx *cctx,
				const unsigned char *in, unsigned len,
				unsigned char *out);
};

extern const struct isrcry_mode_desc _isrcry_ecb_desc;
extern const struct isrcry_mode_desc _isrcry_cbc_desc;

struct isrcry_pad_desc {
	enum isrcry_result (*pad)(unsigned char *buf, unsigned blocklen,
			unsigned datalen);
	enum isrcry_result (*unpad)(unsigned char *buf, unsigned blocklen,
			unsigned *datalen);
};

extern const struct isrcry_pad_desc _isrcry_pkcs5_desc;

struct isrcry_cipher_ctx {
	const struct isrcry_cipher_desc *cipher;
	const struct isrcry_mode_desc *mode;
	void *key;
	unsigned char iv[MAX_BLOCK_LEN];
	enum isrcry_direction direction;
};

struct isrcry_hash_desc {
	void (*init)(struct isrcry_hash_ctx *hctx);
	void (*update)(struct isrcry_hash_ctx *hctx,
				const unsigned char *buffer, unsigned length);
	void (*final)(struct isrcry_hash_ctx *ctx, unsigned char *digest);
	unsigned block_size;
	unsigned digest_size;
	unsigned ctxlen;
};

struct isrcry_hash_ctx {
	const struct isrcry_hash_desc *desc;
	void *ctx;
};

extern const struct isrcry_hash_desc _isrcry_sha1_desc;
extern const struct isrcry_hash_desc _isrcry_md5_desc;

struct isrcry_mac_desc {
	void *(*alloc)(struct isrcry_mac_ctx *mctx);
	enum isrcry_result (*init)(struct isrcry_mac_ctx *mctx,
				const unsigned char *key, unsigned keylen);
	void (*update)(struct isrcry_mac_ctx *mctx,
				const unsigned char *buffer, unsigned length);
	enum isrcry_result (*final)(struct isrcry_mac_ctx *mctx,
				unsigned char *out, unsigned outlen);
	void (*free)(struct isrcry_mac_ctx *mctx);
	enum isrcry_hash hash;
	unsigned mac_size;
};

extern const struct isrcry_mac_desc _isrcry_hmac_sha1_desc;

struct isrcry_mac_ctx {
	const struct isrcry_mac_desc *desc;
	void *key;
	unsigned keylen;
	gboolean inited;
	void *ctx;
};

struct isrcry_compress_desc {
	gboolean can_stream;
	enum isrcry_result (*alloc)(struct isrcry_compress_ctx *cctx);
	enum isrcry_result (*compress_process)(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen);
	enum isrcry_result (*compress_final)(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen);
	enum isrcry_result (*decompress_process)(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen);
	enum isrcry_result (*decompress_final)(
				struct isrcry_compress_ctx *cctx,
				const unsigned char *in, unsigned *inlen,
				unsigned char *out, unsigned *outlen);
	void (*free)(struct isrcry_compress_ctx *cctx);
};

extern const struct isrcry_compress_desc _isrcry_zlib_desc;
extern const struct isrcry_compress_desc _isrcry_lzf_desc;
extern const struct isrcry_compress_desc _isrcry_lzf_stream_desc;
extern const struct isrcry_compress_desc _isrcry_lzma_desc;

struct isrcry_compress_ctx {
	const struct isrcry_compress_desc *desc;
	enum isrcry_direction direction;
	int level;
	void *ctx;
};

/* The helper macros below are originally from libtomcrypt. */

/* Extract a byte portably */
#define byte(x, n) (((x) >> (8 * (n))) & 255)

/* On amd64, gcc will auto-vectorize byte-by-byte loops using the XMM
   registers, and we don't want to defeat that. */
#if defined(HAVE_X86_32)
#define ISRCRY_FAST_TYPE unsigned long
#endif

#define STORE16H(x, y)				\
	do {					\
		uint16_t __t = (x);		\
		__t = GUINT16_TO_BE(__t);	\
		memcpy((y), &__t, 2);		\
	} while (0)

#define LOAD16H(x, y)				\
	do {					\
		uint16_t __t;			\
		memcpy(&__t, (y), 2);		\
		x = GUINT16_FROM_BE(__t);	\
	} while (0)

#define STORE32H(x, y)				\
	do {					\
		uint32_t __t = (x);		\
		__t = GUINT32_TO_BE(__t);	\
		memcpy((y), &__t, 4);		\
	} while (0)

#define LOAD32H(x, y)				\
	do {					\
		uint32_t __t;			\
		memcpy(&__t, (y), 4);		\
		x = GUINT32_FROM_BE(__t);	\
	} while (0)

#define STORE32L(x, y)				\
	do {					\
		uint32_t __t = (x);		\
		__t = GUINT32_TO_LE(__t);	\
		memcpy((y), &__t, 4);		\
	} while (0)

#define LOAD32L(x, y)				\
	do {					\
		uint32_t __t;			\
		memcpy(&__t, (y), 4);		\
		x = GUINT32_FROM_LE(__t);	\
	} while (0)

#endif
