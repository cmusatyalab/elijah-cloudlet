/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * SHA1 hash algorithm, generic compression function
 * Originally from Nettle
 *
 * Copyright (C) 2001, 2004 Peter Gutmann, Andrew Kuchling, Niels Möller
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

#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

/* A block, treated as a sequence of 32-bit words. */
#define SHA1_DATA_LENGTH 16

/* The SHA f()-functions.  The f1 and f3 functions can be optimized to
   save one boolean operation each - thanks to Rich Schroeppel,
   rcs@cs.arizona.edu for discovering this */

#define f1(x,y,z)   ( z ^ ( x & ( y ^ z ) ) )           /* Rounds  0-19 */
#define f2(x,y,z)   ( x ^ y ^ z )                       /* Rounds 20-39 */
#define f3(x,y,z)   ( ( x & y ) | ( z & ( x | y ) ) )   /* Rounds 40-59 */
#define f4(x,y,z)   ( x ^ y ^ z )                       /* Rounds 60-79 */

/* The SHA Mysterious Constants */

#define K1  0x5A827999L                                 /* Rounds  0-19 */
#define K2  0x6ED9EBA1L                                 /* Rounds 20-39 */
#define K3  0x8F1BBCDCL                                 /* Rounds 40-59 */
#define K4  0xCA62C1D6L                                 /* Rounds 60-79 */

/* 32-bit rotate left - kludged with shifts */

#define ROTL(n,X)  ( ( (X) << (n) ) | ( (X) >> ( 32 - (n) ) ) )

/* The initial expanding function.  The hash function is defined over an
   80-word expanded input array W, where the first 16 are copies of the input
   data, and the remaining 64 are defined by

        W[ i ] = W[ i - 16 ] ^ W[ i - 14 ] ^ W[ i - 8 ] ^ W[ i - 3 ]

   This implementation generates these values on the fly in a circular
   buffer - thanks to Colin Plumb, colin@nyx10.cs.du.edu for this
   optimization. */

#define expand(W,i) ( W[ i & 15 ] = \
		      ROTL( 1, ( W[ i & 15 ] ^ W[ (i - 14) & 15 ] ^ \
				 W[ (i - 8) & 15 ] ^ W[ (i - 3) & 15 ] ) ) )


/* The prototype SHA sub-round.  The fundamental sub-round is:

        a' = e + ROTL( 5, a ) + f( b, c, d ) + k + data;
        b' = a;
        c' = ROTL( 30, b );
        d' = c;
        e' = d;

   but this is implemented by unrolling the loop 5 times and renaming the
   variables ( e, a, b, c, d ) = ( a', b', c', d', e' ) each iteration.
   This code is then replicated 20 times for each of the 4 functions, using
   the next 20 values from the W[] array each time */

#define subRound(a, b, c, d, e, f, k, data) \
    ( e += ROTL( 5, a ) + f( b, c, d ) + k + data, b = ROTL( 30, b ) )

void _isrcry_sha1_compress(uint32_t *state, const uint8_t *input)
{
  uint32_t data[16];
  uint32_t A, B, C, D, E;     /* Local vars */
  int i;

  for (i = 0; i < 16; i++, input+= 4)
    LOAD32H(data[i], input);

  /* Set up first buffer and local data buffer */
  A = state[0];
  B = state[1];
  C = state[2];
  D = state[3];
  E = state[4];

  /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
  subRound( A, B, C, D, E, f1, K1, data[ 0] );
  subRound( E, A, B, C, D, f1, K1, data[ 1] );
  subRound( D, E, A, B, C, f1, K1, data[ 2] );
  subRound( C, D, E, A, B, f1, K1, data[ 3] );
  subRound( B, C, D, E, A, f1, K1, data[ 4] );
  subRound( A, B, C, D, E, f1, K1, data[ 5] );
  subRound( E, A, B, C, D, f1, K1, data[ 6] );
  subRound( D, E, A, B, C, f1, K1, data[ 7] );
  subRound( C, D, E, A, B, f1, K1, data[ 8] );
  subRound( B, C, D, E, A, f1, K1, data[ 9] );
  subRound( A, B, C, D, E, f1, K1, data[10] );
  subRound( E, A, B, C, D, f1, K1, data[11] );
  subRound( D, E, A, B, C, f1, K1, data[12] );
  subRound( C, D, E, A, B, f1, K1, data[13] );
  subRound( B, C, D, E, A, f1, K1, data[14] );
  subRound( A, B, C, D, E, f1, K1, data[15] );
  subRound( E, A, B, C, D, f1, K1, expand( data, 16 ) );
  subRound( D, E, A, B, C, f1, K1, expand( data, 17 ) );
  subRound( C, D, E, A, B, f1, K1, expand( data, 18 ) );
  subRound( B, C, D, E, A, f1, K1, expand( data, 19 ) );

  subRound( A, B, C, D, E, f2, K2, expand( data, 20 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 21 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 22 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 23 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 24 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 25 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 26 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 27 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 28 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 29 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 30 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 31 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 32 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 33 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 34 ) );
  subRound( A, B, C, D, E, f2, K2, expand( data, 35 ) );
  subRound( E, A, B, C, D, f2, K2, expand( data, 36 ) );
  subRound( D, E, A, B, C, f2, K2, expand( data, 37 ) );
  subRound( C, D, E, A, B, f2, K2, expand( data, 38 ) );
  subRound( B, C, D, E, A, f2, K2, expand( data, 39 ) );

  subRound( A, B, C, D, E, f3, K3, expand( data, 40 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 41 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 42 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 43 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 44 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 45 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 46 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 47 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 48 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 49 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 50 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 51 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 52 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 53 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 54 ) );
  subRound( A, B, C, D, E, f3, K3, expand( data, 55 ) );
  subRound( E, A, B, C, D, f3, K3, expand( data, 56 ) );
  subRound( D, E, A, B, C, f3, K3, expand( data, 57 ) );
  subRound( C, D, E, A, B, f3, K3, expand( data, 58 ) );
  subRound( B, C, D, E, A, f3, K3, expand( data, 59 ) );

  subRound( A, B, C, D, E, f4, K4, expand( data, 60 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 61 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 62 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 63 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 64 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 65 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 66 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 67 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 68 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 69 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 70 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 71 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 72 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 73 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 74 ) );
  subRound( A, B, C, D, E, f4, K4, expand( data, 75 ) );
  subRound( E, A, B, C, D, f4, K4, expand( data, 76 ) );
  subRound( D, E, A, B, C, f4, K4, expand( data, 77 ) );
  subRound( C, D, E, A, B, f4, K4, expand( data, 78 ) );
  subRound( B, C, D, E, A, f4, K4, expand( data, 79 ) );

  /* Build message digest */
  state[0] += A;
  state[1] += B;
  state[2] += C;
  state[3] += D;
  state[4] += E;
}
