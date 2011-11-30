/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * MD5 hash algorithm
 * Originally from Nettle
 *
 * Copyright (C) 2001 Niels Möller
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

/* Based on public domain code hacked by Colin Plumb, Andrew Kuchling, and
 * Niels Möller. */

#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

#define MD5_DATA_SIZE 64
#define MD5_DIGEST_SIZE 16

struct isrcry_md5_ctx {
	uint32_t digest[MD5_DIGEST_SIZE / 4];
	uint64_t count;
	uint8_t block[MD5_DATA_SIZE];
	unsigned index;
};

/* @state points to 4 u32 words, and @data points to 64 bytes of input data,
   possibly unaligned. */
void _isrcry_md5_compress(uint32_t *state, const uint8_t *data);

static void md5_init(struct isrcry_hash_ctx *hctx)
{
  struct isrcry_md5_ctx *ctx = hctx->ctx;
  
  ctx->digest[0] = 0x67452301;
  ctx->digest[1] = 0xefcdab89;
  ctx->digest[2] = 0x98badcfe;
  ctx->digest[3] = 0x10325476;
  
  ctx->count = 0;
  ctx->index = 0;
}

static void md5_update(struct isrcry_hash_ctx *hctx,
			const unsigned char *buffer, unsigned length)
{
  struct isrcry_md5_ctx *ctx = hctx->ctx;
  
  if (ctx->index)
    {
      /* Try to fill partial block */
      unsigned left = MD5_DATA_SIZE - ctx->index;
      if (length < left)
	{
	  memcpy(ctx->block + ctx->index, buffer, length);
	  ctx->index += length;
	  return; /* Finished */
	}
      else
	{
	  memcpy(ctx->block + ctx->index, buffer, left);

	  _isrcry_md5_compress(ctx->digest, ctx->block);
	  ctx->count++;
	  
	  buffer += left;
	  length -= left;
	}
    }
  while (length >= MD5_DATA_SIZE)
    {
      _isrcry_md5_compress(ctx->digest, buffer);
      ctx->count++;

      buffer += MD5_DATA_SIZE;
      length -= MD5_DATA_SIZE;
    }
  if ((ctx->index = length))
    /* Buffer leftovers */
    memcpy(ctx->block, buffer, length);
}

static void md5_final(struct isrcry_hash_ctx *hctx, unsigned char *digest)
{
  struct isrcry_md5_ctx *ctx = hctx->ctx;
  uint64_t bitcount;
  unsigned i;
  
  /* Final wrapup - pad to MD5_DATA_SIZE-byte boundary with the bit
   * pattern 1 0* (64-bit count of bits processed, LSB-first) */
  i = ctx->index;
  
  /* Set the first char of padding to 0x80. This is safe since there
   * is always at least one byte free */
  g_assert(i < MD5_DATA_SIZE);
  ctx->block[i++] = 0x80;

  if (i > (MD5_DATA_SIZE - 8))
    {
      /* No room for length in this block. Process it and
	 pad with another one */
      memset(ctx->block + i, 0, MD5_DATA_SIZE - i);
      
      _isrcry_md5_compress(ctx->digest, ctx->block);
      i = 0;
    }
  if (i < (MD5_DATA_SIZE - 8))
    memset(ctx->block + i, 0, (MD5_DATA_SIZE - 8) - i);
    
  /* There are 512 = 2^9 bits in one block 
   * Little-endian order => Least significant word first */
  bitcount = (ctx->count << 9) | (ctx->index << 3);
  STORE32L((uint32_t) bitcount, ctx->block + (MD5_DATA_SIZE - 8));
  STORE32L((uint32_t)(bitcount >> 32), ctx->block + (MD5_DATA_SIZE - 4));
  
  _isrcry_md5_compress(ctx->digest, ctx->block);
  
  /* Little endian order */
  for (i = 0; i < MD5_DIGEST_SIZE / 4; i++, digest += 4)
    STORE32L(ctx->digest[i], digest);
}

const struct isrcry_hash_desc _isrcry_md5_desc = {
	.init = md5_init,
	.update = md5_update,
	.final = md5_final,
	.block_size = MD5_DATA_SIZE,
	.digest_size = MD5_DIGEST_SIZE,
	.ctxlen = sizeof(struct isrcry_md5_ctx)
};
