/*
 * libisrutil - utility library for the OpenISR (R) system
 *
 * Copyright (C) 2010 Carnegie Mellon University
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#ifndef LIBISRUTIL_H
#define LIBISRUTIL_H

#include <glib.h>

/* glib log domain:
 *	isrutil
 * Log levels:
 *	G_LOG_LEVEL_CRITICAL		- Programmer errors
 *	G_LOG_LEVEL_MESSAGE		- Ordinary errors
 */

/* Chunk crypto suites and compression types.  Unlike the
   cipher/mode/hash/compression types defined by libisrcrypto,

	>>>>>>>> THESE ARE INCLUDED IN ON-DISK FORMATS, <<<<<<<<

   so don't renumber them! */

enum iu_chunk_crypto {
	IU_CHUNK_CRY_UNKNOWN = 0,
	/* 1 = blowfish-sha1, obsolete */
	IU_CHUNK_CRY_AES_SHA1 = 2
};

enum iu_chunk_compress {
	IU_CHUNK_COMP_UNKNOWN = 0,
	IU_CHUNK_COMP_NONE = 1,
	IU_CHUNK_COMP_ZLIB = 2,
	IU_CHUNK_COMP_LZF = 3
};

/* Chunk */

enum iu_chunk_crypto iu_chunk_crypto_parse(const char *desc);
gboolean iu_chunk_crypto_is_valid(enum iu_chunk_crypto type);
unsigned iu_chunk_crypto_hashlen(enum iu_chunk_crypto type);
gboolean iu_chunk_crypto_digest(enum iu_chunk_crypto crypto, void *out,
			const void *in, unsigned len);

enum iu_chunk_compress iu_chunk_compress_parse(const char *desc);
gboolean iu_chunk_compress_is_enabled(unsigned enabled_map,
			enum iu_chunk_compress type);

gboolean iu_chunk_encode(enum iu_chunk_crypto crypto,
			const void *in, unsigned inlen,
			void *out, unsigned *outlen, void *tag, void *key,
			enum iu_chunk_compress *compress);
gboolean iu_chunk_decode(enum iu_chunk_crypto crypto,
			enum iu_chunk_compress compress, unsigned chunk,
			const void *in, unsigned inlen, const void *key,
			void *out, unsigned outlen);

#endif
