/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * SHA1 hash algorithm, AMD64 compression function
 * Originally from Nettle
 * Ported from M4 to cpp by Benjamin Gilbert <bgilbert@cs.cmu.edu>
 * Ported from IA32 to AMD64 by Benjamin Gilbert
 *
 * Copyright (C) 2004, Niels Möller
 * Copyright (C) 2006-2008 Carnegie Mellon University
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

#include "config.h"

/* Register usage.  r12-15 must be saved if they will be used.  Accessing
   r8-r15 takes an extra instruction byte. */
#define P_STATE	%rdi			/* Pointer parameter */
#define P_DATA	%rsi			/* Pointer parameter */
#define SA	%edi			/* Reuses P_STATE */
#define SB	%esi			/* Reuses P_DATA */
#define SC	%eax
#define SD	%ebx			/* Callee-saved */
#define SE	%ebp			/* Callee-saved */
#define TMP	%ecx
#define TMP2	%r8d			/* Used by F3 */
#define QTMP	%rcx			/* 64-bit version of TMP for CLEAR() */
#define CONST	%edx
#define STATE	%r9
#define DATA	%rsp			/* Scratch space pointer */

/* Constants */
#define K1VALUE	$0x5A827999		/* Rounds  0-19 */
#define K2VALUE	$0x6ED9EBA1		/* Rounds 20-39 */
#define K3VALUE	$0x8F1BBCDC		/* Rounds 40-59 */
#define K4VALUE	$0xCA62C1D6		/* Rounds 60-79 */

/* Convert stack offsets in 32-bit words to offsets in bytes */
#define OFFSET(i) 4*(i)

/* Reads the input via P_DATA into register, byteswaps it, and stores it in
   the DATA array. */
#define SWAP(index, register)					\
	movl	OFFSET(index)(P_DATA), register;		\
	bswap	register;					\
	movl	register, OFFSET(index)(DATA)

/*
 * expand(i) is the expansion function
 *
 *   W[i] = (W[i - 16] ^ W[i - 14] ^ W[i - 8] ^ W[i - 3]) <<< 1
 *
 * where W[i] is stored in DATA[i mod 16].
 *
 * Result is stored back in W[i], and also left in TMP, the only
 * register that is used.
 */
#define EXPAND(i)						\
	movl	OFFSET(i % 16)(DATA), TMP;			\
	xorl	OFFSET((i + 2) % 16)(DATA), TMP;		\
	xorl	OFFSET((i + 8) % 16)(DATA), TMP;		\
	xorl	OFFSET((i + 13) % 16)(DATA), TMP;		\
	roll	$1, TMP;					\
	movl	TMP, OFFSET(i % 16)(DATA)

/*
 * The f functions,
 *
 *  f1(x,y,z) = z ^ (x & (y ^ z))
 *  f2(x,y,z) = x ^ y ^ z
 *  f3(x,y,z) = (x & y) | (z & (x | y))
 *  f4 = f2
 *
 * The macro Fk(x,y,z) computes = fk(x,y,z). 
 * Result is left in TMP.
 */
#define F1(x,y,z)						\
	movl	z, TMP;						\
	xorl	y, TMP;						\
	andl	x, TMP;						\
	xorl	z, TMP

#define F2(x,y,z)						\
	movl	x, TMP;						\
	xorl	y, TMP;						\
	xorl	z, TMP

#define F3(x,y,z)						\
	movl	x, TMP2;					\
	andl	y, TMP2;					\
	movl	x, TMP;						\
	orl	y, TMP;						\
	andl	z, TMP;						\
	orl	TMP2, TMP

/*
 * The form of one sha1 round is
 *
 *   a' = e + a <<< 5 + f( b, c, d ) + k + w;
 *   b' = a;
 *   c' = b <<< 30;
 *   d' = c;
 *   e' = d;
 *
 * where <<< denotes rotation. We permute our variables, so that we
 * instead get
 *
 *   e += a <<< 5 + f( b, c, d ) + k + w;
 *   b <<<= 30
 *
 * k is not an explicit parameter; it's always stored in CONST.
 *
 * Using the TMP register for the rotate could be avoided, by rotating
 * %a in place, adding, and then rotating back.
 */
#define ROUND(a,b,c,d,e,f,w)					\
	addl	CONST, e;					\
	addl	w, e;						\
	f(b,c,d);						\
	addl	TMP, e;						\
	movl	a, TMP;						\
	roll	$5, TMP;					\
	addl	TMP, e;						\
	roll	$30, b;

/* _isrcry_sha1_compress(u32 *state, u8 *data) */
.text
.align 4
.globl _isrcry_sha1_compress
#ifdef HAVE_VISIBILITY
.hidden _isrcry_sha1_compress
#endif
_isrcry_sha1_compress:
	push	%rbx
	push	%rbp
	subq	$64, %rsp	/* %rsp = W */

	/* Load and byteswap data */
	SWAP( 0,  %eax); SWAP( 1,  %ebx); SWAP( 2,  %ecx); SWAP( 3,  %edx)
	SWAP( 4,  %ebp); SWAP( 5,  %r8d); SWAP( 6,  %r9d); SWAP( 7, %r10d)
	SWAP( 8,  %eax); SWAP( 9,  %ebx); SWAP(10,  %ecx); SWAP(11,  %edx)
	SWAP(12,  %ebp); SWAP(13,  %r8d); SWAP(14,  %r9d); SWAP(15, %r10d)

	/* P_DATA now dead; free up P_STATE for other uses */
	movq	P_STATE, STATE

	/* load the state vector */
	movl	(STATE),   SA
	movl	4(STATE),  SB
	movl	8(STATE),  SC
	movl	12(STATE), SD
	movl	16(STATE), SE

	movl	K1VALUE, CONST	
	ROUND(SA, SB, SC, SD, SE, F1, OFFSET( 0)(DATA))
	ROUND(SE, SA, SB, SC, SD, F1, OFFSET( 1)(DATA))
	ROUND(SD, SE, SA, SB, SC, F1, OFFSET( 2)(DATA))
	ROUND(SC, SD, SE, SA, SB, F1, OFFSET( 3)(DATA))
	ROUND(SB, SC, SD, SE, SA, F1, OFFSET( 4)(DATA))

	ROUND(SA, SB, SC, SD, SE, F1, OFFSET( 5)(DATA))
	ROUND(SE, SA, SB, SC, SD, F1, OFFSET( 6)(DATA))
	ROUND(SD, SE, SA, SB, SC, F1, OFFSET( 7)(DATA))
	ROUND(SC, SD, SE, SA, SB, F1, OFFSET( 8)(DATA))
	ROUND(SB, SC, SD, SE, SA, F1, OFFSET( 9)(DATA))

	ROUND(SA, SB, SC, SD, SE, F1, OFFSET(10)(DATA))
	ROUND(SE, SA, SB, SC, SD, F1, OFFSET(11)(DATA))
	ROUND(SD, SE, SA, SB, SC, F1, OFFSET(12)(DATA))
	ROUND(SC, SD, SE, SA, SB, F1, OFFSET(13)(DATA))
	ROUND(SB, SC, SD, SE, SA, F1, OFFSET(14)(DATA))

	ROUND(SA, SB, SC, SD, SE, F1, OFFSET(15)(DATA))
	EXPAND(16); ROUND(SE, SA, SB, SC, SD, F1, TMP)
	EXPAND(17); ROUND(SD, SE, SA, SB, SC, F1, TMP)
	EXPAND(18); ROUND(SC, SD, SE, SA, SB, F1, TMP)
	EXPAND(19); ROUND(SB, SC, SD, SE, SA, F1, TMP)

	movl	K2VALUE, CONST
	EXPAND(20); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(21); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(22); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(23); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(24); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(25); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(26); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(27); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(28); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(29); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(30); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(31); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(32); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(33); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(34); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(35); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(36); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(37); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(38); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(39); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	movl	K3VALUE, CONST
	EXPAND(40); ROUND(SA, SB, SC, SD, SE, F3, TMP)
	EXPAND(41); ROUND(SE, SA, SB, SC, SD, F3, TMP)
	EXPAND(42); ROUND(SD, SE, SA, SB, SC, F3, TMP)
	EXPAND(43); ROUND(SC, SD, SE, SA, SB, F3, TMP)
	EXPAND(44); ROUND(SB, SC, SD, SE, SA, F3, TMP)

	EXPAND(45); ROUND(SA, SB, SC, SD, SE, F3, TMP)
	EXPAND(46); ROUND(SE, SA, SB, SC, SD, F3, TMP)
	EXPAND(47); ROUND(SD, SE, SA, SB, SC, F3, TMP)
	EXPAND(48); ROUND(SC, SD, SE, SA, SB, F3, TMP)
	EXPAND(49); ROUND(SB, SC, SD, SE, SA, F3, TMP)

	EXPAND(50); ROUND(SA, SB, SC, SD, SE, F3, TMP)
	EXPAND(51); ROUND(SE, SA, SB, SC, SD, F3, TMP)
	EXPAND(52); ROUND(SD, SE, SA, SB, SC, F3, TMP)
	EXPAND(53); ROUND(SC, SD, SE, SA, SB, F3, TMP)
	EXPAND(54); ROUND(SB, SC, SD, SE, SA, F3, TMP)

	EXPAND(55); ROUND(SA, SB, SC, SD, SE, F3, TMP)
	EXPAND(56); ROUND(SE, SA, SB, SC, SD, F3, TMP)
	EXPAND(57); ROUND(SD, SE, SA, SB, SC, F3, TMP)
	EXPAND(58); ROUND(SC, SD, SE, SA, SB, F3, TMP)
	EXPAND(59); ROUND(SB, SC, SD, SE, SA, F3, TMP)

	movl	K4VALUE, CONST
	EXPAND(60); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(61); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(62); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(63); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(64); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(65); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(66); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(67); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(68); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(69); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(70); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(71); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(72); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(73); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(74); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	EXPAND(75); ROUND(SA, SB, SC, SD, SE, F2, TMP)
	EXPAND(76); ROUND(SE, SA, SB, SC, SD, F2, TMP)
	EXPAND(77); ROUND(SD, SE, SA, SB, SC, F2, TMP)
	EXPAND(78); ROUND(SC, SD, SE, SA, SB, F2, TMP)
	EXPAND(79); ROUND(SB, SC, SD, SE, SA, F2, TMP)

	/* Update the state vector */
	addl	SA, (STATE) 
	addl	SB, 4(STATE) 
	addl	SC, 8(STATE) 
	addl	SD, 12(STATE) 
	addl	SE, 16(STATE)

	addq	$64, %rsp
	pop	%rbp
	pop	%rbx
	ret

/* Mark this object file as not needing an executable stack */
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
