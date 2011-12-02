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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "isrcrypto.h"
#include "vectors.h"
#include "vectors_aes.h"
#include "vectors_sha1.h"
#include "vectors_md5.h"
#include "vectors_hmac.h"
#include "vectors_compress.h"

int failed;

#define fail(fmt, args...) do {\
		printf("%s failed " fmt "\n", __func__, ## args); \
		failed++; \
	} while (0)

void ecb_test(const char *alg, enum isrcry_cipher type,
			const struct ecb_test *vectors, unsigned vec_count)
{
	struct isrcry_cipher_ctx *ctx;
	const struct ecb_test *test;
	enum isrcry_result ret;
	unsigned char buf[32];
	unsigned n;
	unsigned blocksize;

	ctx = isrcry_cipher_alloc(type, ISRCRY_MODE_ECB);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	blocksize = isrcry_cipher_block(type);
	if (blocksize < 8 || blocksize > 16)
		fail("%s invalid blocksize", alg);
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		ret = isrcry_cipher_init(ctx, ISRCRY_ENCRYPT, test->key,
					test->keylen, NULL);
		if (ret) {
			fail("%s %u encrypt init %i", alg, n, ret);
			continue;
		}
		ret = isrcry_cipher_process(ctx, test->plain, blocksize, buf);
		if (ret)
			fail("%s %u encrypt %d", alg, n, ret);
		if (memcmp(buf, test->cipher, blocksize))
			fail("%s %u encrypt mismatch", alg, n);

		ret = isrcry_cipher_init(ctx, ISRCRY_DECRYPT, test->key,
					test->keylen, NULL);
		if (ret) {
			fail("%s %u decrypt init %i", alg, n, ret);
			continue;
		}
		ret = isrcry_cipher_process(ctx, test->cipher, blocksize, buf);
		if (ret)
			fail("%s %u decrypt %d", alg, n, ret);
		if (memcmp(buf, test->plain, blocksize))
			fail("%s %u decrypt mismatch", alg, n);
	}
	isrcry_cipher_free(ctx);
}

void chain_test(const char *alg, enum isrcry_cipher type,
			enum isrcry_mode mode,
			const struct chain_test *vectors, unsigned vec_count)
{
	struct isrcry_cipher_ctx *ctx;
	const struct chain_test *test;
	enum isrcry_result ret;
	unsigned char buf[1024];
	unsigned n;
	unsigned blocksize;

	ctx = isrcry_cipher_alloc(type, mode);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	blocksize = isrcry_cipher_block(type);
	if (blocksize < 8 || blocksize > 16)
		fail("%s invalid blocksize", alg);
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		ret = isrcry_cipher_init(ctx, ISRCRY_ENCRYPT, test->key,
					test->keylen, test->iv);
		if (ret) {
			fail("%s %u encrypt init %d", alg, n, ret);
			continue;
		}
		ret = isrcry_cipher_process(ctx, test->plain, test->plainlen,
					buf);
		if (ret)
			fail("%s %u encrypt %d", alg, n, ret);
		if (memcmp(buf, test->cipher, test->plainlen))
			fail("%s %u encrypt mismatch", alg, n);

		ret = isrcry_cipher_init(ctx, ISRCRY_DECRYPT, test->key,
					test->keylen, test->iv);
		if (ret) {
			fail("%s %u decrypt init %d", alg, n, ret);
			continue;
		}
		ret = isrcry_cipher_process(ctx, test->cipher, test->plainlen,
					buf);
		if (ret)
			fail("%s %u decrypt %d", alg, n, ret);
		if (memcmp(buf, test->plain, test->plainlen))
			fail("%s %u decrypt mismatch", alg, n);
	}
	isrcry_cipher_free(ctx);
}

void monte_test(const char *alg, enum isrcry_cipher type,
			const struct monte_test *vectors, unsigned vec_count)
{
	struct isrcry_cipher_ctx *ctx;
	const struct monte_test *test;
	unsigned n;
	unsigned m;
	unsigned l;
	uint8_t key[32];
	uint8_t buf[64];
	uint8_t *in;
	uint8_t *out;
	enum isrcry_result ret;
	unsigned blocksize;

	ctx = isrcry_cipher_alloc(type, ISRCRY_MODE_ECB);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	blocksize = isrcry_cipher_block(type);
	if (blocksize < 8 || blocksize > 16)
		fail("%s invalid blocksize", alg);
	in = buf;
	out = buf + blocksize;
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		memset(key, 0, test->keylen);
		memset(buf, 0, sizeof(buf));
		for (m = 0; m < test->ngroups; m++) {
			ret = isrcry_cipher_init(ctx, test->encrypt ?
						ISRCRY_ENCRYPT : ISRCRY_DECRYPT,
						key, test->keylen, NULL);
			if (ret) {
				fail("%s %u init %u", alg, n, m);
				break;
			}
			for (l = 0; l < test->niters; l++) {
				memcpy(in, out, blocksize);
				ret = isrcry_cipher_process(ctx, in,
							blocksize, out);
				if (ret) {
					fail("%s %u crypt %u %u", alg, n, m, l);
					break;
				}
				/* buf now holds the last two ciphertexts */
			}
			for (l = 0; l < test->keylen; l++)
				key[l] ^= buf[l + 32 - test->keylen];
		}
		if (memcmp(out, test->out, blocksize))
			fail("%s %u result mismatch", alg, n);
	}
	isrcry_cipher_free(ctx);
}

void hash_test(const char *alg, enum isrcry_hash type,
			const struct hash_test *vectors, unsigned vec_count)
{
	struct isrcry_hash_ctx *ctx;
	const struct hash_test *test;
	uint8_t out[64];
	unsigned n;
	unsigned hashlen;

	ctx = isrcry_hash_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	hashlen = isrcry_hash_len(type);
	if (hashlen < 16 || hashlen > 64)
		fail("%s invalid hashlen", alg);
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		isrcry_hash_update(ctx, test->data, test->len);
		isrcry_hash_final(ctx, out);
		if (memcmp(out, test->hash, hashlen))
			fail("%s %u result mismatch", alg, n);
	}
	isrcry_hash_free(ctx);
}

void hash_simple_monte_test(const char *alg, enum isrcry_hash type,
			const struct hash_monte_test *vectors,
			unsigned vec_count)
{
	struct isrcry_hash_ctx *ctx;
	const struct hash_monte_test *test;
	uint8_t buf[64];
	unsigned n;
	unsigned m;
	unsigned hashlen;

	ctx = isrcry_hash_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	hashlen = isrcry_hash_len(type);
	if (hashlen < 16 || hashlen > 64)
		fail("%s invalid hashlen", alg);
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		memcpy(buf, test->seed, hashlen);
		if (test->ngroups != 1)
			fail("%s %u invalid vector", alg, n);
		for (m = 0; m < test->niters; m++) {
			isrcry_hash_update(ctx, buf, hashlen);
			isrcry_hash_final(ctx, buf);
		}
		if (memcmp(buf, test->hash, hashlen))
			fail("%s %u result mismatch", alg, n);
	}
	isrcry_hash_free(ctx);
}

void hash_monte_test(const char *alg, enum isrcry_hash type,
			const struct hash_monte_test *vectors,
			unsigned vec_count)
{
	struct isrcry_hash_ctx *ctx;
	const struct hash_monte_test *test;
	uint8_t buf[192];
	uint8_t *out;
	unsigned n;
	unsigned m;
	unsigned l;
	unsigned hashlen;

	ctx = isrcry_hash_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	hashlen = isrcry_hash_len(type);
	if (hashlen < 16 || hashlen > 64)
		fail("%s invalid hashlen", alg);
	out = buf + 2 * hashlen;
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		memcpy(out, test->seed, hashlen);
		for (m = 0; m < test->ngroups; m++) {
			for (l = 0; l < 2; l++)
				memcpy(buf + l * hashlen, out, hashlen);
			for (l = 0; l < test->niters; l++) {
				isrcry_hash_update(ctx, buf, 3 * hashlen);
				memmove(buf, buf + hashlen, 2 * hashlen);
				isrcry_hash_final(ctx, out);
			}
		}
		if (memcmp(out, test->hash, hashlen))
			fail("%s %u result mismatch", alg, n);
	}
	isrcry_hash_free(ctx);
}

void mac_test(const char *alg, enum isrcry_mac type,
			const struct mac_test *vectors, unsigned vec_count)
{
	struct isrcry_mac_ctx *ctx;
	const struct mac_test *test;
	uint8_t mac[64 + 1];
	unsigned n;
	unsigned m;

	ctx = isrcry_mac_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		if (isrcry_mac_init(ctx, test->key, test->keylen)) {
			fail("%s init %u", alg, n);
			continue;
		}
		for (m = 0; m < 2; m++) {
			if (isrcry_mac_update(ctx, test->data, test->datalen)) {
				fail("%s update %u %u", alg, n, m);
				continue;
			}
			mac[test->maclen] = 0xc1;
			if (isrcry_mac_final(ctx, mac, test->maclen)) {
				fail("%s final %u %u", alg, n, m);
				continue;
			}
			if (memcmp(mac, test->mac, test->maclen)) {
				fail("%s result %u %u", alg, n, m);
				continue;
			}
			if (mac[test->maclen] != 0xc1)
				fail("%s overrun %u %u", alg, n, m);
		}
	}
	isrcry_mac_free(ctx);
}

unsigned compress_stream(const char *alg, const char *direction, unsigned test,
			struct isrcry_compress_ctx *ctx, const void *inbuf,
			unsigned inlen, void *outbuf)
{
	unsigned in_count;
	unsigned out_count;
	unsigned in_offset = 0;
	unsigned out_offset = 0;
	enum isrcry_result ret;

	while (in_offset < inlen) {
		in_count = out_count = 1;
		if (isrcry_compress_process(ctx, inbuf + in_offset,
					&in_count, outbuf + out_offset,
					&out_count)) {
			fail("%s %s stream-process %u", alg, direction, test);
			return 0;
		}
		if (in_count == 0 && out_count == 0) {
			fail("%s %s stream-progress %u", alg, direction, test);
			return 0;
		}
		in_offset += in_count;
		out_offset += out_count;
	}
	do {
		in_count = 0;
		out_count = 1;
		ret = isrcry_compress_final(ctx, inbuf + in_offset, &in_count,
					outbuf + out_offset, &out_count);
		out_offset += out_count;
	} while (ret == ISRCRY_BUFFER_OVERFLOW);
	if (ret != ISRCRY_OK) {
		fail("%s %s stream-final %u", alg, direction, test);
		return 0;
	}
	return out_offset;
}

void compress_test(const char *alg, enum isrcry_compress type,
			const struct compress_test *vectors,
			unsigned vec_count)
{
	struct isrcry_compress_ctx *ctx;
	const struct compress_test *test;
	uint8_t *plain;
	uint8_t *compress;
	unsigned buflen;
	unsigned inlen;
	unsigned outlen;
	unsigned n;

	ctx = isrcry_compress_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		buflen = 2 * (test->plainlen + test->compresslen);
		plain = malloc(buflen);
		compress = malloc(buflen);

		/* Test a round-trip of the plaintext. */
		memset(compress, 0, buflen);
		if (isrcry_compress_init(ctx, ISRCRY_ENCODE, test->level)) {
			fail("%s init %u", alg, n);
			continue;
		}
		inlen = test->plainlen;
		outlen = buflen;
		if (isrcry_compress_final(ctx, test->plain, &inlen, compress,
					&outlen)) {
			fail("%s compress %u", alg, n);
			continue;
		}
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
			continue;
		}
		inlen = outlen;
		outlen = buflen;
		if (isrcry_compress_final(ctx, compress, &inlen, plain,
					&outlen)) {
			fail("%s decompress %u", alg, n);
			continue;
		}
		if (outlen != test->plainlen) {
			fail("%s plainlen %u", alg, n);
			continue;
		}
		if (memcmp(plain, test->plain, test->plainlen)) {
			fail("%s compress mismatch %u", alg, n);
			continue;
		}

		/* Test a streaming round-trip of the plaintext. */
		if (isrcry_compress_can_stream(type)) {
			memset(compress, 0, buflen);
			if (isrcry_compress_init(ctx, ISRCRY_ENCODE,
						test->level)) {
				fail("%s init %u", alg, n);
				continue;
			}
			inlen = compress_stream(alg, "encode", n, ctx,
						test->plain, test->plainlen,
						compress);
			if (inlen == 0)
				continue;
			if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
				fail("%s init %u", alg, n);
				continue;
			}
			outlen = compress_stream(alg, "decode", n, ctx,
						compress, inlen, plain);
			if (outlen == 0)
				continue;
			if (outlen != test->plainlen) {
				fail("%s stream-plainlen %u", alg, n);
				continue;
			}
			if (memcmp(plain, test->plain, test->plainlen)) {
				fail("%s stream-compress mismatch %u", alg, n);
				continue;
			}
		} else {
			if (isrcry_compress_init(ctx, ISRCRY_ENCODE,
						test->level)) {
				fail("%s init %u", alg, n);
				continue;
			}
			inlen = test->plainlen;
			outlen = buflen;
			if (isrcry_compress_process(ctx, test->plain, &inlen,
						compress, &outlen) !=
						ISRCRY_NO_STREAMING) {
				fail("%s no-streaming %u", alg, n);
				continue;
			}
			if (inlen != 0 || outlen != 0) {
				fail("%s false-progress-streaming %u", alg, n);
				continue;
			}
		}

		/* Test a decode of the compresstext. */
		memset(plain, 0, buflen);
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
       			continue;
		}
		inlen = test->compresslen;
		outlen = buflen;
		if (isrcry_compress_final(ctx, test->compress, &inlen, plain,
					&outlen)) {
			fail("%s decompress-prepared %u", alg, n);
			continue;
		}
		if (outlen != test->plainlen) {
			fail("%s prepared-plainlen %u", alg, n);
			continue;
		}
		if (memcmp(plain, test->plain, test->plainlen)) {
			fail("%s prepared mismatch %u", alg, n);
			continue;
		}

		/* Test trailing garbage on decode. */
		memcpy(compress, test->compress, test->compresslen);
		compress[test->compresslen] = 12;
		memset(plain, 0, buflen);
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
       			continue;
		}
		inlen = test->compresslen + 1;
		outlen = buflen;
		if (isrcry_compress_final(ctx, test->compress, &inlen, plain,
					&outlen) != ISRCRY_BAD_FORMAT) {
			fail("%s trailing-garbage %u", alg, n);
			continue;
		}

		/* Test overflow detection. */
		if (isrcry_compress_init(ctx, ISRCRY_ENCODE, test->level)) {
			fail("%s init %u", alg, n);
       			continue;
		}
		inlen = test->plainlen;
		outlen = 1;
		if (isrcry_compress_final(ctx, test->plain, &inlen, compress,
					&outlen) != ISRCRY_BUFFER_OVERFLOW) {
			fail("%s overflow-compress %u", alg, n);
			continue;
		}
		if (!isrcry_compress_can_stream(type) && (inlen || outlen)) {
			fail("%s false-progress-compress %u", alg, n);
			continue;
		}
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
       			continue;
		}
		inlen = test->compresslen;
		outlen = test->plainlen - 1;
		if (isrcry_compress_final(ctx, test->compress, &inlen, plain,
					&outlen) != ISRCRY_BUFFER_OVERFLOW) {
			fail("%s overflow-decompress %u", alg, n);
			continue;
		}
		if (!isrcry_compress_can_stream(type) && (inlen || outlen)) {
			fail("%s false-progress-decompress %u", alg, n);
			continue;
		}

		free(plain);
		free(compress);
	}
	isrcry_compress_free(ctx);
}

void decompress_test(const char *alg, enum isrcry_compress type,
			const struct decompress_test *vectors,
			unsigned vec_count)
{
	struct isrcry_compress_ctx *ctx;
	const struct decompress_test *test;
	uint8_t out[64];
	unsigned inlen;
	unsigned outlen;
	unsigned n;
	int ret;

	ctx = isrcry_compress_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	for (n = 0; n < vec_count; n++) {
		test = &vectors[n];
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
			continue;
		}
		inlen = test->len;
		outlen = sizeof(out);
		ret = isrcry_compress_final(ctx, test->data, &inlen, out,
					&outlen);
		if (ret && test->success) {
			fail("%s fail-decomp %u", alg, n);
			continue;
		}
		if (!ret && !test->success) {
			fail("%s xfail-decomp %u", alg, n);
			continue;
		}
		if (!ret && inlen != test->len) {
			fail("%s incomplete-decomp %u", alg, n);
			continue;
		}
	}
	isrcry_compress_free(ctx);
}

unsigned min(unsigned a, unsigned b)
{
	return a < b ? a : b;
}

void compress_stream_fuzz_test(const char *alg, enum isrcry_compress type,
			unsigned count)
{
	struct isrcry_compress_ctx *ctx;
	struct isrcry_random_ctx *rctx;
	uint8_t plain[131100];
	uint8_t compress[140000];
	unsigned compress_size;
	uint8_t out[140000];
	unsigned in_offset;
	unsigned out_offset;
	unsigned inlen;
	unsigned outlen;
	unsigned n;
	int ret;

	/* Initialize plaintext */
	for (n = 0; n < sizeof(plain); n++)
		plain[n] = n;
	rctx = isrcry_random_alloc();
	isrcry_random_bytes(rctx, plain + 65536, 65536);
	isrcry_random_free(rctx);

	ctx = isrcry_compress_alloc(type);
	if (ctx == NULL) {
		fail("%s alloc", alg);
		return;
	}
	/* Obtain canonical compresstext */
	if (isrcry_compress_init(ctx, ISRCRY_ENCODE, 0)) {
		fail("%s init", alg);
		isrcry_compress_free(ctx);
		return;
	}
	inlen = sizeof(plain);
	outlen = sizeof(compress);
	if (isrcry_compress_final(ctx, plain, &inlen, compress, &outlen)) {
		fail("%s compress-fuzz", alg);
		isrcry_compress_free(ctx);
		return;
	}
	compress_size = outlen;

	/* Fuzz compression buffering */
	for (n = 0; n < count; n++) {
		srandom(n);
		if (isrcry_compress_init(ctx, ISRCRY_ENCODE, 0)) {
			fail("%s init %u", alg, n);
			continue;
		}
		in_offset = out_offset = 0;
		while (in_offset < sizeof(plain)) {
			inlen = min(random() % 8, sizeof(plain) - in_offset);
			outlen = min(random() % 8, sizeof(out) - out_offset);
			if (isrcry_compress_process(ctx, plain + in_offset,
						&inlen, out + out_offset,
						&outlen)) {
				fail("%s compress-fuzz %u", alg, n);
				goto out;
			}
			in_offset += inlen;
			out_offset += outlen;
		}
		while (1) {
			inlen = 0;
			outlen = min(random() % 8, sizeof(out) - out_offset);
			ret = isrcry_compress_final(ctx, NULL, &inlen,
						out + out_offset, &outlen);
			out_offset += outlen;
			if (ret == ISRCRY_OK)
				break;
			if (ret != ISRCRY_BUFFER_OVERFLOW) {
				fail("%s compress-fuzz-final %u", alg, n);
				goto out;
			}
		}
		if (out_offset != compress_size) {
			fail("%s compress-fuzz-size-mismatch %u", alg, n);
			goto out;
		}
		if (memcmp(compress, out, compress_size)) {
			fail("%s compress-fuzz-mismatch %u", alg, n);
			goto out;
		}
	}

	/* Fuzz decompression buffering */
	for (n = 0; n < count; n++) {
		srandom(n);
		if (isrcry_compress_init(ctx, ISRCRY_DECODE, 0)) {
			fail("%s init %u", alg, n);
			continue;
		}
		in_offset = out_offset = 0;
		while (in_offset < compress_size) {
			inlen = min(random() % 8, compress_size - in_offset);
			outlen = min(random() % 8, sizeof(out) - out_offset);
			if (isrcry_compress_process(ctx, compress + in_offset,
						&inlen, out + out_offset,
						&outlen)) {
				fail("%s decompress-fuzz %u", alg, n);
				goto out;
			}
			in_offset += inlen;
			out_offset += outlen;
		}
		while (1) {
			inlen = 0;
			outlen = min(random() % 8, sizeof(out) - out_offset);
			ret = isrcry_compress_final(ctx, NULL, &inlen,
						out + out_offset, &outlen);
			out_offset += outlen;
			if (ret == ISRCRY_OK)
				break;
			if (ret != ISRCRY_BUFFER_OVERFLOW) {
				fail("%s decompress-fuzz-final %u", alg, n);
				goto out;
			}
		}
		if (out_offset != sizeof(plain)) {
			fail("%s decompress-fuzz-size-mismatch %u", alg, n);
			goto out;
		}
		if (memcmp(plain, out, sizeof(plain))) {
			fail("%s decompress-fuzz-mismatch %u", alg, n);
			goto out;
		}
	}
out:
	isrcry_compress_free(ctx);
}

/* Statistical random number generator tests defined in
 * FIPS 140-1 - 4.11.1 Power-Up Tests.  Originally from RPC2.
 *
 * A single bit stream of 20,000 consecutive bits of output from the
 * generator is subjected to each of the following tests. If any of the
 * tests fail, then the module shall enter an error state.
 *
 * The Monobit Test
 *  1. Count the number of ones in the 20,000 bit stream. Denote this
 *     quantity by X.
 *  2. The test is passed if 9,654 < X < 10,346
 *
 * The Poker Test
 *  1. Divide the 20,000 bit stream into 5,000 contiguous 4 bit
 *     segments. Count and store the number of occurrences of each of
 *     the 16 possible 4 bit values. Denote f(i) as the number of each 4
 *     bit value i where 0 < i < 15.
 *  2. Evaluate the following: X = (16/5000) * (Sum[f(i)]^2)-5000
 *  3. The test is passed if 1.03 < X < 57.4
 *
 * The Runs Test
 *  1. A run is defined as a maximal sequence of consecutive bits of
 *     either all ones or all zeros, which is part of the 20,000 bit
 *     sample stream. The incidences of runs (for both consecutive zeros
 *     and consecutive ones) of all lengths ( 1) in the sample stream
 *     should be counted and stored.
 *  2. The test is passed if the number of runs that occur (of lengths 1
 *     through 6) is each within the corresponding interval specified
 *     below. This must hold for both the zeros and ones; that is, all
 *     12 counts must lie in the specified interval. For the purpose of
 *     this test, runs of greater than 6 are considered to be of length 6.
 *       Length of Run			    Required Interval
 *	     1					2,267-2,733
 *	     2					1,079-1,421
 *	     3					502-748
 *	     4					223-402
 *	     5					90-223
 *	     6+					90-223
 *
 * The Long Run Test
 *  1. A long run is defined to be a run of length 34 or more (of either
 *     zeros or ones).
 *  2. On the sample of 20,000 bits, the test is passed if there are NO
 *     long runs.
 */
void random_fips_test(void)
{
	struct isrcry_random_ctx *ctx;
	uint32_t data[20000 / (sizeof(uint32_t) * 8)];
	uint32_t val;
	unsigned i, j, idx;
	int ones, f[16], run, odd, longrun;

	ctx = isrcry_random_alloc();
	if (ctx == NULL) {
		fail("random alloc");
		return;
	}
	isrcry_random_bytes(ctx, data, sizeof(data));
	isrcry_random_free(ctx);

	/* Monobit Test */
	for (ones = 0, i = 0 ; i < sizeof(data)/sizeof(data[0]); i++) {
		val = data[i];
		while (val) {
			if (val & 1)
				ones++;
			val >>= 1;
		}
	}
	if (ones <= 9654 || ones >= 10346)
		fail("random monobit");

	/* Poker Test */
	memset(f, 0, sizeof(f));
	for (i = 0; i < sizeof(data)/sizeof(data[0]); i++) {
		for (j = 0; j < 32; j += 4) {
			idx = (data[i] >> j) & 0xf;
			f[idx]++;
		}
	}
	for (val = 0, i = 0; i < 16; i++)
		val += f[i] * f[i];
	if ((val & 0xf0000000) || (val << 4) <= 25005150 ||
				(val << 4) >= 25287000)
		fail("random poker");

	/* Runs Test */
	memset(f, 0, sizeof(f));
	odd = run = longrun = 0;
	for (i = 0 ; i < sizeof(data)/sizeof(data[0]); i++) {
		val = data[i];
		for (j = 0; j < 32; j++) {
			if (odd ^ (val & 1)) {
				if (run) {
					if (run > longrun)
						longrun = run;
					if (run > 6)
						run = 6;
					idx = run - 1 + (odd ? 6 : 0);
					f[idx]++;
				}
				odd = val & 1;
				run = 0;
			}
			run++;
			val >>= 1;
		}
	}
	if (run > longrun)
		longrun = run;
	if (run > 6)
		run = 6;
	idx = run - 1 + (odd ? 6 : 0);
	f[idx]++;

	if (f[0] <= 2267 || f[0] >= 2733 || f[6] <= 2267 || f[6] >= 2733 ||
		 f[1] <= 1079 || f[1] >= 1421 || f[7] <= 1079 || f[7] >= 1421 ||
		 f[2] <= 502  || f[2] >= 748  || f[8] <= 502  || f[8] >= 748 ||
		 f[3] <= 223  || f[3] >= 402  || f[9] <= 223  || f[9] >= 402 ||
		 f[4] <= 90   || f[4] >= 223  || f[10] <= 90  || f[10] >= 223 ||
		 f[5] <= 90   || f[5] >= 223  || f[11] <= 90  || f[11] >= 223)
		fail("random runs");
	if (longrun >= 34)
		fail("random long runs");
}

int main(void)
{
	ecb_test("aes", ISRCRY_CIPHER_AES, aes_ecb_vectors,
				MEMBERS(aes_ecb_vectors));
	monte_test("aes", ISRCRY_CIPHER_AES, aes_monte_vectors,
				MEMBERS(aes_monte_vectors));
	chain_test("aes", ISRCRY_CIPHER_AES, ISRCRY_MODE_CBC,
				aes_cbc_vectors, MEMBERS(aes_cbc_vectors));
	hash_test("sha1", ISRCRY_HASH_SHA1, sha1_hash_vectors,
				MEMBERS(sha1_hash_vectors));
	hash_monte_test("sha1", ISRCRY_HASH_SHA1, sha1_monte_vectors,
				MEMBERS(sha1_monte_vectors));
	hash_test("md5", ISRCRY_HASH_MD5, md5_hash_vectors,
				MEMBERS(md5_hash_vectors));
	hash_simple_monte_test("md5", ISRCRY_HASH_MD5, md5_monte_vectors,
				MEMBERS(md5_monte_vectors));
	mac_test("hmac-sha1", ISRCRY_MAC_HMAC_SHA1, hmac_sha1_vectors,
				MEMBERS(hmac_sha1_vectors));
	compress_test("zlib", ISRCRY_COMPRESS_ZLIB, zlib_compress_vectors,
				MEMBERS(zlib_compress_vectors));
	compress_test("lzf", ISRCRY_COMPRESS_LZF, lzf_compress_vectors,
				MEMBERS(lzf_compress_vectors));
	compress_test("lzf-stream", ISRCRY_COMPRESS_LZF_STREAM,
				lzf_stream_compress_vectors,
				MEMBERS(lzf_stream_compress_vectors));
	decompress_test("lzf-stream", ISRCRY_COMPRESS_LZF_STREAM,
				lzf_stream_decompress_vectors,
				MEMBERS(lzf_stream_decompress_vectors));
	compress_stream_fuzz_test("lzf-stream", ISRCRY_COMPRESS_LZF_STREAM, 5);
	compress_test("lzma", ISRCRY_COMPRESS_LZMA, lzma_compress_vectors,
				MEMBERS(lzma_compress_vectors));
	random_fips_test();

	if (failed) {
		printf("%d tests failed\n", failed);
		return 1;
	} else {
		printf("All tests passed\n");
		return 0;
	}
}
