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

#ifndef ISRCRY_TEST_VECTORS_H
#define ISRCRY_TEST_VECTORS_H

#include <stdint.h>

#define MEMBERS(a) (sizeof(a)/sizeof((a)[0]))

struct ecb_test {
	uint8_t key[32];
	uint8_t plain[16];
	uint8_t cipher[16];
	unsigned keylen;
};

struct chain_test {
	uint8_t key[16];
	uint8_t iv[16];
	uint8_t plain[128];
	uint8_t cipher[128];
	unsigned plainlen;
	unsigned keylen;
};

struct monte_test {
	uint8_t out[16];
	unsigned keylen;
	unsigned ngroups;
	unsigned niters;
	int encrypt;
};

struct hash_test {
	uint8_t data[512];
	unsigned len;
	uint8_t hash[64];
};

struct hash_monte_test {
	uint8_t seed[64];
	unsigned ngroups;
	unsigned niters;
	uint8_t hash[64];
};

struct mac_test {
	uint8_t key[80];
	unsigned keylen;
	uint8_t data[80];
	unsigned datalen;
	uint8_t mac[20];
	unsigned maclen;
};

struct rsa_test_key {
	uint8_t modulus[257];
	unsigned modulus_len;
	uint8_t publicExponent[3];
	unsigned publicExponent_len;
	uint8_t privateExponent[257];
	unsigned privateExponent_len;
	uint8_t prime1[129];
	unsigned prime1_len;
	uint8_t prime2[129];
	unsigned prime2_len;
	uint8_t exponent1[129];
	unsigned exponent1_len;
	uint8_t exponent2[129];
	unsigned exponent2_len;
	uint8_t coefficient[129];
	unsigned coefficient_len;
};

struct rsa_sign_test {
	const struct rsa_test_key *key;
	uint8_t data[256];
	unsigned datalen;
	uint8_t salt[20];
	unsigned saltlen;
	uint8_t sig[256];
	unsigned siglen;
};

struct compress_test {
	int level;
	const uint8_t *plain;
	unsigned plainlen;
	const uint8_t *compress;
	unsigned compresslen;
};

struct decompress_test {
	int success;  /* should the decompression succeed? */
	unsigned len;
	uint8_t data[32];
};

#endif
