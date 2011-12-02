/*
 * libisrutil - utility library for the OpenISR (R) system
 *
 * Copyright (C) 2006-2010 Carnegie Mellon University
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

#include <string.h>
#include "isrcrypto.h"
#define LIBISRUTIL_INTERNAL
#include "internal.h"

/* Crypto */

exported enum iu_chunk_crypto iu_chunk_crypto_parse(const char *desc)
{
	if (!strcmp(desc, "aes-sha1"))
		return IU_CHUNK_CRY_AES_SHA1;
	return IU_CHUNK_CRY_UNKNOWN;
}

static gboolean crypto_get_algs(enum iu_chunk_crypto crypto,
			enum isrcry_cipher *_cipher, enum isrcry_mode *_mode,
			enum isrcry_padding *_padding, enum isrcry_hash *_hash,
			unsigned *_keylen)
{
	enum isrcry_cipher cipher;
	enum isrcry_mode mode;
	enum isrcry_padding padding;
	enum isrcry_hash hash;
	unsigned keylen;

	switch (crypto) {
	case IU_CHUNK_CRY_AES_SHA1:
		cipher = ISRCRY_CIPHER_AES;
		mode = ISRCRY_MODE_CBC;
		padding = ISRCRY_PADDING_PKCS5;
		hash = ISRCRY_HASH_SHA1;
		keylen = 16;
		break;
	default:
		return FALSE;
	}
	if (_cipher)
		*_cipher = cipher;
	if (_mode)
		*_mode = mode;
	if (_padding)
		*_padding = padding;
	if (_hash)
		*_hash = hash;
	if (_keylen)
		*_keylen = keylen;
	return TRUE;
}

exported gboolean iu_chunk_crypto_is_valid(enum iu_chunk_crypto type)
{
	return crypto_get_algs(type, NULL, NULL, NULL, NULL, NULL);
}

exported unsigned iu_chunk_crypto_hashlen(enum iu_chunk_crypto type)
{
	enum isrcry_hash alg;

	if (!crypto_get_algs(type, NULL, NULL, NULL, &alg, NULL))
		return 0;
	return isrcry_hash_len(alg);
}

exported gboolean iu_chunk_crypto_digest(enum iu_chunk_crypto crypto,
			void *out, const void *in, unsigned len)
{
	struct isrcry_hash_ctx *ctx;
	enum isrcry_hash alg;

	if (!crypto_get_algs(crypto, NULL, NULL, NULL, &alg, NULL)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Invalid crypto suite requested");
		return FALSE;
	}
	ctx = isrcry_hash_alloc(alg);
	if (ctx == NULL) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Couldn't allocate digest context");
		return FALSE;
	}
	isrcry_hash_update(ctx, in, len);
	isrcry_hash_final(ctx, out);
	isrcry_hash_free(ctx);
	return TRUE;
}

/* Compress */

exported enum iu_chunk_compress iu_chunk_compress_parse(const char *desc)
{
	if (!strcmp(desc, "none"))
		return IU_CHUNK_COMP_NONE;
	if (!strcmp(desc, "zlib"))
		return IU_CHUNK_COMP_ZLIB;
	if (!strcmp(desc, "lzf"))
		return IU_CHUNK_COMP_LZF;
	return IU_CHUNK_COMP_UNKNOWN;
}

static gboolean compress_to_isrcry(enum iu_chunk_crypto in,
			enum isrcry_compress *out)
{
	/* IU_CHUNK_COMP_NONE must be handled specially by the caller */
	switch (in) {
	case IU_CHUNK_COMP_ZLIB:
		*out = ISRCRY_COMPRESS_ZLIB;
		return TRUE;
	case IU_CHUNK_COMP_LZF:
		*out = ISRCRY_COMPRESS_LZF;
		return TRUE;
	default:
		return FALSE;
	}
}

exported gboolean iu_chunk_compress_is_enabled(unsigned enabled_map,
			enum iu_chunk_compress type)
{
	if (type <= IU_CHUNK_COMP_UNKNOWN || type >= 8 * sizeof(enabled_map))
		return FALSE;
	return !!(enabled_map & (1 << type));
}

/* Encode/decode */

exported gboolean iu_chunk_encode(enum iu_chunk_crypto crypto,
			const void *in, unsigned inlen, void *out,
			unsigned *outlen, void *tag, void *key,
			enum iu_chunk_compress *compress)
{
	enum isrcry_cipher cipher;
	enum isrcry_mode mode;
	enum isrcry_padding padding;
	enum isrcry_hash hash;
	enum isrcry_compress cry_compress;
	unsigned keylen;
	struct isrcry_cipher_ctx *cipher_ctx;
	struct isrcry_hash_ctx *hash_ctx;
	struct isrcry_compress_ctx *compress_ctx;
	void *compressed = NULL;
	enum isrcry_result rc;
	unsigned plainlen;
	unsigned compresslen;
	unsigned cipher_block;

	/* Get algorithm parameters */
	if (!crypto_get_algs(crypto, &cipher, &mode, &padding, &hash,
				&keylen)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
				"Invalid crypto suite %d", crypto);
		return FALSE;
	}
	cipher_block = isrcry_cipher_block(cipher);
	if (*compress != IU_CHUNK_COMP_NONE &&
			!compress_to_isrcry(*compress, &cry_compress)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
				"Invalid compression algorithm %d", *compress);
		return FALSE;
	}

	/* Compress chunk */
	if (*compress != IU_CHUNK_COMP_NONE) {
		compress_ctx = isrcry_compress_alloc(cry_compress);
		if (compress_ctx == NULL) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Couldn't allocate compression "
					"algorithm %d", cry_compress);
			return FALSE;
		}
		rc = isrcry_compress_init(compress_ctx, ISRCRY_ENCODE, 0);
		if (rc) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Failed to initialize compressor: %s",
					isrcry_strerror(rc));
			isrcry_compress_free(compress_ctx);
			return FALSE;
		}
		plainlen = inlen;
		compresslen = inlen;
		compressed = g_malloc(inlen);
		rc = isrcry_compress_final(compress_ctx, in, &plainlen,
					compressed, &compresslen);
		if (rc || compresslen >= inlen - cipher_block) {
			/* Compression failed or didn't save enough space
			   to be worthwhile (after accounting for cipher
			   padding); store uncompressed. */
			*compress = IU_CHUNK_COMP_NONE;
			g_free(compressed);
			compressed = NULL;
		}
		isrcry_compress_free(compress_ctx);
	}

	/* Calculate key */
	hash_ctx = isrcry_hash_alloc(hash);
	if (hash_ctx == NULL) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't allocate "
				"hash algorithm %d", hash);
		g_free(compressed);
		return FALSE;
	}
	isrcry_hash_update(hash_ctx, compressed ?: in,
				compressed ? compresslen : inlen);
	isrcry_hash_final(hash_ctx, key);

	/* Encrypt chunk */
	cipher_ctx = isrcry_cipher_alloc(cipher, mode);
	if (cipher_ctx == NULL) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't allocate "
				"cipher algorithm %d", cipher);
		isrcry_hash_free(hash_ctx);
		g_free(compressed);
		return FALSE;
	}
	rc = isrcry_cipher_init(cipher_ctx, ISRCRY_ENCRYPT, key, keylen, NULL);
	if (rc) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't "
				"initialize cipher: %s", isrcry_strerror(rc));
		isrcry_cipher_free(cipher_ctx);
		isrcry_hash_free(hash_ctx);
		g_free(compressed);
		return FALSE;
	}
	if (compressed) {
		*outlen = inlen;
		rc = isrcry_cipher_final(cipher_ctx, padding, compressed,
					compresslen, out, outlen);
		g_free(compressed);
	} else {
		rc = isrcry_cipher_process(cipher_ctx, in, inlen, out);
		*outlen = inlen;
	}
	isrcry_cipher_free(cipher_ctx);
	if (rc) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't run "
				"cipher: %s", isrcry_strerror(rc));
		isrcry_hash_free(hash_ctx);
		return FALSE;
	}

	/* Calculate tag */
	isrcry_hash_update(hash_ctx, out, *outlen);
	isrcry_hash_final(hash_ctx, tag);
	isrcry_hash_free(hash_ctx);

	return TRUE;
}

exported gboolean iu_chunk_decode(enum iu_chunk_crypto crypto,
			enum iu_chunk_compress compress, unsigned chunk,
			const void *in, unsigned inlen, const void *key,
			void *out, unsigned outlen)
{
	enum isrcry_cipher cipher;
	enum isrcry_mode mode;
	enum isrcry_padding padding;
	enum isrcry_hash hash;
	enum isrcry_compress cry_compress;
	unsigned keylen;
	unsigned hashlen;
	struct isrcry_cipher_ctx *cipher_ctx;
	struct isrcry_hash_ctx *hash_ctx;
	struct isrcry_compress_ctx *compress_ctx;
	void *calc_hash;
	void *compressed = NULL;
	unsigned compresslen;
	unsigned plainlen;
	gboolean is_compressed = (compress != IU_CHUNK_COMP_NONE);
	enum isrcry_result rc;

	/* Get algorithm parameters */
	if (!crypto_get_algs(crypto, &cipher, &mode, &padding, &hash,
				&keylen)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
				"Invalid crypto suite %d", crypto);
		return FALSE;
	}
	hashlen = isrcry_hash_len(hash);
	if (is_compressed && !compress_to_isrcry(compress, &cry_compress)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
				"Invalid compression algorithm %d", compress);
		return FALSE;
	}

	/* Sanity checks */
	if ((!is_compressed && inlen != outlen) || inlen == 0 ||
				inlen > outlen) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Invalid length "
				"%u for chunk %u", inlen, chunk);
		return FALSE;
	}

	/* We don't check the chunk tag because the cipher-padding and key
	   checks will find anything the tag check might find. */

	/* Decrypt chunk */
	cipher_ctx = isrcry_cipher_alloc(cipher, mode);
	if (cipher_ctx == NULL) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't allocate "
				"cipher algorithm %d", cipher);
		return FALSE;
	}
	rc = isrcry_cipher_init(cipher_ctx, ISRCRY_DECRYPT, key, keylen, NULL);
	if (rc) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't "
				"initialize cipher: %s", isrcry_strerror(rc));
		isrcry_cipher_free(cipher_ctx);
		return FALSE;
	}
	if (is_compressed) {
		compressed = g_malloc(outlen);
		compresslen = outlen;
		rc = isrcry_cipher_final(cipher_ctx, padding, in, inlen,
					compressed, &compresslen);
	} else {
		rc = isrcry_cipher_process(cipher_ctx, in, inlen, out);
	}
	isrcry_cipher_free(cipher_ctx);
	if (rc) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, "Failed to "
				"decrypt chunk %u: %s", chunk,
				isrcry_strerror(rc));
		g_free(compressed);
		return FALSE;
	}

	/* Check key against decrypted data */
	hash_ctx = isrcry_hash_alloc(hash);
	if (hash_ctx == NULL) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "Couldn't allocate "
				"hash algorithm %d", hash);
		g_free(compressed);
		return FALSE;
	}
	calc_hash = g_malloc(hashlen);
	isrcry_hash_update(hash_ctx, is_compressed ? compressed : out,
				is_compressed ? compresslen : outlen);
	isrcry_hash_final(hash_ctx, calc_hash);
	isrcry_hash_free(hash_ctx);
	if (memcmp(key, calc_hash, hashlen)) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
				"Bad key for chunk %u", chunk);
		g_free(calc_hash);
		g_free(compressed);
		return FALSE;
	}
	g_free(calc_hash);

	/* Decompress chunk */
	if (is_compressed) {
		compress_ctx = isrcry_compress_alloc(cry_compress);
		if (compress_ctx == NULL) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Couldn't allocate compressor");
			g_free(compressed);
			return FALSE;
		}
		rc = isrcry_compress_init(compress_ctx, ISRCRY_DECODE, 0);
		if (rc) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
					"Failed to initialize decompressor: "
					"%s", isrcry_strerror(rc));
			isrcry_compress_free(compress_ctx);
			g_free(compressed);
			return FALSE;
		}
		plainlen = outlen;
		rc = isrcry_compress_final(compress_ctx, compressed,
					&compresslen, out, &plainlen);
		isrcry_compress_free(compress_ctx);
		g_free(compressed);
		if (rc) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
					"Failed to decompress chunk %u: %s",
					chunk, isrcry_strerror(rc));
			return FALSE;
		}
		if (plainlen != outlen) {
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
					"Invalid decoded chunk length %u "
					"on chunk %u", plainlen, chunk);
			return FALSE;
		}
	}

	return TRUE;
}
