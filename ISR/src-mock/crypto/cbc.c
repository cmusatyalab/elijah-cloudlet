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

/* This file is adapted from libtomcrypt, whose license block follows. */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.com
 */

#include <stdlib.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

/**
  CBC encrypt
  @param cctx     Cipher context
  @param in       Plaintext
  @param len      The number of bytes to process (must be multiple of block length)
  @param out      [out] Ciphertext
  @return ISRCRY_OK if successful
*/
static enum isrcry_result cbc_encrypt(struct isrcry_cipher_ctx *cctx,
			const unsigned char *in, unsigned len,
			unsigned char *out)
{
   unsigned blocklen = cctx->cipher->blocklen;
   unsigned char *iv = cctx->iv;
   unsigned x;
   enum isrcry_result err;

   if (in == NULL || out == NULL)
	   return ISRCRY_INVALID_ARGUMENT;
   if (blocklen < 1 || len % blocklen)
	   return ISRCRY_INVALID_ARGUMENT;
#ifdef ISRCRY_FAST_TYPE
   if (blocklen % sizeof(ISRCRY_FAST_TYPE))
	   return ISRCRY_INVALID_ARGUMENT;
#endif

   while (len) {
      /* xor IV against plaintext */
#ifdef ISRCRY_FAST_TYPE
      for (x = 0; x < blocklen; x += sizeof(ISRCRY_FAST_TYPE)) {
	  *((ISRCRY_FAST_TYPE*)(iv + x)) ^= *((ISRCRY_FAST_TYPE*)(in + x));
      }
#else
      for (x = 0; x < blocklen; x++) {
          iv[x] ^= in[x];
      }
#endif

       /* encrypt */
      if ((err = cctx->cipher->encrypt(cctx, iv, out)) != ISRCRY_OK) {
	  return err;
      }

      /* store IV [ciphertext] for a future block */
#ifdef ISRCRY_FAST_TYPE
      for (x = 0; x < blocklen; x += sizeof(ISRCRY_FAST_TYPE)) {
	  *((ISRCRY_FAST_TYPE*)((unsigned char *)iv + x)) = *((ISRCRY_FAST_TYPE*)((unsigned char *)out + x));
      }
#else
      for (x = 0; x < blocklen; x++) {
	  iv[x] = out[x];
      }
#endif
      
      out += blocklen;
      in  += blocklen;
      len -= blocklen;
   }
   return ISRCRY_OK;
}

/**
  CBC decrypt
  @param cctx     Cipher context
  @param in       Ciphertext
  @param len      The number of bytes to process (must be multiple of block length)
  @param out      [out] Plaintext
  @return ISRCRY_OK if successful
*/
static enum isrcry_result cbc_decrypt(struct isrcry_cipher_ctx *cctx,
			const unsigned char *in, unsigned len,
			unsigned char *out)
{
   unsigned blocklen = cctx->cipher->blocklen;
   unsigned char *iv = cctx->iv;
   unsigned x;
   enum isrcry_result err;
   unsigned char tmp[16];
#ifdef ISRCRY_FAST_TYPE
   ISRCRY_FAST_TYPE tmpy;
#else
   unsigned char tmpy;
#endif         

   if (in == NULL || out == NULL)
	   return ISRCRY_INVALID_ARGUMENT;
   if (blocklen < 1 || len % blocklen)
	   return ISRCRY_INVALID_ARGUMENT;
#ifdef ISRCRY_FAST_TYPE
   if (blocklen % sizeof(ISRCRY_FAST_TYPE))
	   return ISRCRY_INVALID_ARGUMENT;
#endif
   
    while (len) {
       /* decrypt */
       if ((err = cctx->cipher->decrypt(cctx, in, tmp)) != ISRCRY_OK)
	       return err;

       /* xor IV against plaintext */
#ifdef ISRCRY_FAST_TYPE
      for (x = 0; x < blocklen; x += sizeof(ISRCRY_FAST_TYPE)) {
          tmpy = *((ISRCRY_FAST_TYPE*)(iv + x)) ^ *((ISRCRY_FAST_TYPE*)(tmp + x));
          *((ISRCRY_FAST_TYPE*)(iv + x)) = *((ISRCRY_FAST_TYPE*)(in + x));
          *((ISRCRY_FAST_TYPE*)(out + x)) = tmpy;
      }
#else 
      for (x = 0; x < blocklen; x++) {
         tmpy       = tmp[x] ^ iv[x];
         iv[x]      = in[x];
         out[x]     = tmpy;
      }
#endif
     
       in  += blocklen;
       out += blocklen;
       len -= blocklen;
   }
   return ISRCRY_OK;
}

const struct isrcry_mode_desc _isrcry_cbc_desc = {
	.encrypt = cbc_encrypt,
	.decrypt = cbc_decrypt
};
