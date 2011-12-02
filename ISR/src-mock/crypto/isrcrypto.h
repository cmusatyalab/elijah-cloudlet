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

#ifndef LIBISRCRYPTO_H
#define LIBISRCRYPTO_H

#include <stdint.h>

enum isrcry_result {
	ISRCRY_OK			= 0,
	ISRCRY_INVALID_ARGUMENT		= 1,
	ISRCRY_BAD_PADDING		= 2,
	ISRCRY_BAD_FORMAT		= 3,
	ISRCRY_BUFFER_OVERFLOW		= 5,
	ISRCRY_NO_STREAMING		= 8,
};

enum isrcry_direction {
	ISRCRY_DECRYPT			= 0,  /* legacy */
	ISRCRY_ENCRYPT			= 1,  /* legacy */
	ISRCRY_DECODE			= 0,
	ISRCRY_ENCODE			= 1
};

enum isrcry_cipher {
	ISRCRY_CIPHER_AES		= 0,
};

enum isrcry_mode {
	ISRCRY_MODE_ECB			= 0,
	ISRCRY_MODE_CBC			= 1,
};

enum isrcry_padding {
	ISRCRY_PADDING_PKCS5		= 0,
};

enum isrcry_hash {
	ISRCRY_HASH_SHA1		= 0,
	ISRCRY_HASH_MD5			= 1,
};

enum isrcry_mac {
	ISRCRY_MAC_HMAC_SHA1		= 0,
};

enum isrcry_compress {
	/* Deflate wrapped in a zlib header/trailer */
	ISRCRY_COMPRESS_ZLIB		= 0,
	/* Raw LZF block, no streaming */
	ISRCRY_COMPRESS_LZF		= 1,
	/* Series of LZF blocks framed as by lzf.c, with a trailing CRC */
	ISRCRY_COMPRESS_LZF_STREAM	= 2,
	/* xz-format LZMA compression */
	ISRCRY_COMPRESS_LZMA		= 3,
};

struct isrcry_cipher_ctx;
struct isrcry_hash_ctx;
struct isrcry_random_ctx;
struct isrcry_mac_ctx;
struct isrcry_compress_ctx;

/***** Cipher functions *****/

/* Allocate a cipher context for the given algorithm and cipher mode.
   Returns NULL on error. */
struct isrcry_cipher_ctx *isrcry_cipher_alloc(enum isrcry_cipher cipher,
			enum isrcry_mode mode);

/* Free the cipher context. */
void isrcry_cipher_free(struct isrcry_cipher_ctx *cctx);

/* Set the cipher direction (encrypt/decrypt), key, and IV for this cipher
   context.  This function may be called more than once.  @iv must be the
   length of one cipher block, or NULL to use an all-zero IV. */
enum isrcry_result isrcry_cipher_init(struct isrcry_cipher_ctx *cctx,
			enum isrcry_direction direction,
			const void *key, int keylen, const void *iv);

/* Process some cipher blocks from @in to @out.  @inlen is in bytes and must
   be a multiple of the block length.  @out must be able to store @inlen
   bytes. */
enum isrcry_result isrcry_cipher_process(struct isrcry_cipher_ctx *cctx,
			const void *in, unsigned inlen, void *out);

/* Process the final cipher block, padding appropriately with the specified
   @padding.  This function should not be called if the application wishes
   to encrypt an exact multiple of the cipher block length without padding.
   @inlen may be greater than the cipher block size.  @outlen is an in/out
   parameter and must be at least (@inlen + 1) rounded up to the next full
   block.  @cctx is not automatically reinitialized; it may be rekeyed with
   isrcry_cipher_init() or used for further cipher operations with
   isrcry_cipher_process()/isrcry_cipher_final(). */
enum isrcry_result isrcry_cipher_final(struct isrcry_cipher_ctx *cctx,
			enum isrcry_padding padding, const void *in,
			unsigned inlen, void *out, unsigned *outlen);

/* Return the cipher block length for the given cipher, in bytes. */
unsigned isrcry_cipher_block(enum isrcry_cipher type);


/***** Hash functions *****/

/* Allocate a hash context for the given algorithm.  Returns NULL on error. */
struct isrcry_hash_ctx *isrcry_hash_alloc(enum isrcry_hash type);

/* Free the hash context. */
void isrcry_hash_free(struct isrcry_hash_ctx *ctx);

/* Read @length bytes from @buffer and mix them into the hash.  This function
   may be called more than once to read data incrementally. */
void isrcry_hash_update(struct isrcry_hash_ctx *ctx,
			const void *buffer, unsigned length);

/* Finalize the hash over the bytes supplied with isrcry_hash_update().
   Write the result into @digest, which must be large enough to contain it.
   @ctx is automatically reinitialized for additional hash operations. */
void isrcry_hash_final(struct isrcry_hash_ctx *ctx, void *digest);

/* Return the digest length for the given hash, in bytes. */
unsigned isrcry_hash_len(enum isrcry_hash type);


/***** MAC functions *****/

/* Allocate a MAC context for the given MAC parameter set.  Returns NULL on
   error. */
struct isrcry_mac_ctx *isrcry_mac_alloc(enum isrcry_mac type);

/* Free the MAC context. */
void isrcry_mac_free(struct isrcry_mac_ctx *mctx);

/* Initialize this MAC context with the given shared secret.  This function
   may be called more than once. */
enum isrcry_result isrcry_mac_init(struct isrcry_mac_ctx *mctx,
			const void *key, unsigned keylen);

/* Read @length bytes from @buffer and mix them into the MAC.  This function
   may be called more than once to read data incrementally. */
enum isrcry_result isrcry_mac_update(struct isrcry_mac_ctx *mctx,
			const void *buffer, unsigned length);

/* Compute a MAC of length @outlen over the bytes supplied with
   isrcry_mac_update(), and write the result into @out.  @outlen may not be
   larger than the maximum length for the MAC algorithm.  @ctx is
   automatically reinitialized for additional MAC operations. */
enum isrcry_result isrcry_mac_final(struct isrcry_mac_ctx *mctx, void *out,
			unsigned outlen);

/* Return the maximum MAC length for the given algorithm. */
unsigned isrcry_mac_len(enum isrcry_mac type);


/***** Random functions *****/

/* Allocate a random number generation context.  Returns NULL on error.
   Unlike the other libisrcrypto modules, isrcry_random_ctx can safely be
   used from multiple threads without external locking. */
struct isrcry_random_ctx *isrcry_random_alloc(void);

/* Write @length random bytes into the given @buffer. */
void isrcry_random_bytes(struct isrcry_random_ctx *rctx, void *buffer,
			unsigned length);

/* Free the random context. */
void isrcry_random_free(struct isrcry_random_ctx *rctx);


/***** Compression functions *****/

/* Allocate a compression context for the given algorithm.  Returns NULL
   on error. */
struct isrcry_compress_ctx *isrcry_compress_alloc(
			enum isrcry_compress compress);

/* Free the compression context. */
void isrcry_compress_free(struct isrcry_compress_ctx *cctx);

/* Prepare the compression context to encode (compress) or decode
   (decompress) data.  @level specifies an algorithm-specific compression
   level, and probably only makes sense on encode.  A @level of zero will
   use the algorithm's default. */
enum isrcry_result isrcry_compress_init(struct isrcry_compress_ctx *cctx,
			enum isrcry_direction direction, int level);

/* Incrementally process up to @inlen bytes of data from @in and produce
   up to @outlen bytes of data in @out.  On return, @inlen will contain the
   number of bytes consumed (which may be fewer than were provided) and
   @outlen will contain the number of bytes produced.  The compression
   algorithm may buffer input data internally; a series of calls to this
   function must be terminated by a call to isrcry_compress_final() to flush
   buffered data.  Not all compression algorithms support incrementally
   processing data; others will return ISRCRY_NO_STREAMING.
   isrcry_compress_can_stream() can be used to determine whether a particular
   algorithm supports incremental processing. */
enum isrcry_result isrcry_compress_process(struct isrcry_compress_ctx *cctx,
			const void *in, unsigned *inlen, void *out,
			unsigned *outlen);

/* Finish processing the data being compressed or decompressed, consuming
   up to @inlen bytes of data from @in and producing up to @outlen bytes of
   data in @out.  On return, @inlen will contain the number of bytes consumed
   and @outlen will contain the number of bytes produced.  If all data has
   been processed, ISRCRY_OK will be returned.  A return value of
   ISRCRY_BUFFER_OVERFLOW indicates that not all input has been consumed
   or not all output has been produced, and the function must be called again
   with more output buffer space.  Non-streaming algorithms must process
   all input and produce all output in a single call to
   isrcry_compress_final(); when these algorithms return
   ISRCRY_BUFFER_OVERFLOW, @inlen and @outlen will always be set to zero.
   After this function completes successfully, the only valid subsequent
   operations on @cctx are isrcry_compress_init() and
   isrcry_compress_free(). */
enum isrcry_result isrcry_compress_final(struct isrcry_compress_ctx *cctx,
			const void *in, unsigned *inlen, void *out,
			unsigned *outlen);

/* Return nonzero if the given algorithm can process data incrementally via
   isrcry_compress_process(), zero otherwise. */
int isrcry_compress_can_stream(enum isrcry_compress compress);


/***** Utility functions *****/

/* Returns a string describing the given error code.  The returned string
   must not be freed by the application. */
const char *isrcry_strerror(enum isrcry_result result);

#endif
