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

/* This file is adapted from liblzf.  The original attributions follow. */

/*
 * Copyright (c) 2000-2008 Marc Alexander Lehmann <schmorp@schmorp.de>
 * 
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 * 
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 * 
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

/***********************************************************************
**
**	lzf -- an extremely fast/free compression/decompression-method
**	http://liblzf.plan9.de/
**
**	This algorithm is believed to be patent-free.
**
***********************************************************************/

#include <string.h>
#include <errno.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

/*
 * Size of hashtable is (1 << HLOG) * sizeof (char *)
 * decompression is independent of the hash table size
 * the difference between 15 and 14 is very small
 * for small blocks (and 14 is usually a bit faster).
 * For a low-memory/faster configuration, use HLOG == 13;
 * For best compression, use 15 or 16 (or more, up to 23).
 */
#define HLOG 13

/*
 * Sacrifice very little compression quality in favour of compression speed.
 * This gives almost the same compression as the default code, and is
 * (very roughly) 15% faster. This is the preferred mode of operation.
 */
#define VERY_FAST 1

/*
 * Sacrifice some more compression quality in favour of compression speed.
 * (roughly 1-2% worse compression for large blocks and
 * 9-10% for small, redundant, blocks and >>20% better speed in both cases)
 * In short: when in need for speed, enable this for binary data,
 * possibly disable this for text data.
 */
#define ULTRA_FAST 1

/*
 * Unconditionally aligning does not cost very much, so do it if unsure
 */
#define STRICT_ALIGN !(defined(__i386) || defined (__amd64))

/*
 * You may choose to pre-set the hash table (might be faster on some
 * modern cpus and large (>>64k) blocks, and also makes compression
 * deterministic/repeatable when the configuration otherwise is the same).
 */
#define INIT_HTAB 1

/*
 * Wether to pass the LZF_STATE variable as argument, or allocate it
 * on the stack. For small-stack environments, define this to 1.
 */
#define LZF_STATE_ARG 1

/*
 * Wether to add extra checks for input validity in lzf_decompress
 * and return EINVAL if the input stream has been corrupted. This
 * only shields against overflowing the input buffer and will not
 * detect most corrupted streams.
 * This check is not normally noticable on modern hardware
 * (<1% slowdown), but might slow down older cpus considerably.
 */
#define CHECK_INPUT 1

/*****************************************************************************/
/* nothing should be changed below */

typedef unsigned char u8;

typedef const u8 *LZF_STATE[1 << (HLOG)];

#if !STRICT_ALIGN
/* for unaligned accesses we need a 16 bit datatype. */
# include <limits.h>
# if USHRT_MAX == 65535
    typedef unsigned short u16;
# elif UINT_MAX == 65535
    typedef unsigned int u16;
# else
#  undef STRICT_ALIGN
#  define STRICT_ALIGN 1
# endif
#endif

#if ULTRA_FAST
# if defined(VERY_FAST)
#  undef VERY_FAST
# endif
#endif

#define HSIZE (1 << (HLOG))

/*
 * don't play with this unless you benchmark!
 * decompression is not dependent on the hash function
 * the hashing function might seem strange, just believe me
 * it works ;)
 */
#ifndef FRST
# define FRST(p) (((p[0]) << 8) | p[1])
# define NEXT(v,p) (((v) << 8) | p[2])
# if ULTRA_FAST
#  define IDX(h) ((( h             >> (3*8 - HLOG)) - h  ) & (HSIZE - 1))
# elif VERY_FAST
#  define IDX(h) ((( h             >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
# else
#  define IDX(h) ((((h ^ (h << 5)) >> (3*8 - HLOG)) - h*5) & (HSIZE - 1))
# endif
#endif
/*
 * IDX works because it is very similar to a multiplicative hash, e.g.
 * ((h * 57321 >> (3*8 - HLOG)) & (HSIZE - 1))
 * the latter is also quite fast on newer CPUs, and compresses similarly.
 *
 * the next one is also quite good, albeit slow ;)
 * (int)(cos(h & 0xffffff) * 1e6)
 */

#if 0
/* original lzv-like hash function, much worse and thus slower */
# define FRST(p) (p[0] << 5) ^ p[1]
# define NEXT(v,p) ((v) << 5) ^ p[2]
# define IDX(h) ((h) & (HSIZE - 1))
#endif

#define        MAX_LIT        (1 <<  5)
#define        MAX_OFF        (1 << 13)
#define        MAX_REF        ((1 << 8) + (1 << 3))

#if __GNUC__ >= 3
# define expect(expr,value)         __builtin_expect ((expr),(value))
# define inline                     inline
#else
# define expect(expr,value)         (expr)
# define inline                     static
#endif

#define expect_false(expr) expect ((expr) != 0, 0)
#define expect_true(expr)  expect ((expr) != 0, 1)

#if (__i386 || __amd64) && __GNUC__ >= 3
# define lzf_movsb(dst, src, len)                \
   asm ("rep movsb"                              \
        : "=D" (dst), "=S" (src), "=c" (len)     \
        :  "0" (dst),  "1" (src),  "2" (len));
#endif

/*
 * compressed format
 *
 * 000LLLLL <L+1>    ; literal
 * LLLooooo oooooooo ; backref L
 * 111ooooo LLLLLLLL oooooooo ; backref L+7
 *
 */

/*
 * Compress in_len bytes stored at the memory block starting at
 * in_data and write the result to out_data, up to a maximum length
 * of out_len bytes.
 *
 * If the output buffer is not large enough or any error occurs return 0,
 * otherwise return the number of bytes used, which might be considerably
 * more than in_len (but less than 104% of the original size), so it
 * makes sense to always use out_len == in_len - 1), to ensure _some_
 * compression, and store the data uncompressed otherwise (with a flag, of
 * course.
 *
 * lzf_compress might use different algorithms on different systems and
 * even different runs, thus might result in different compressed strings
 * depending on the phase of the moon or similar factors. However, all
 * these strings are architecture-independent and will result in the
 * original data when decompressed using lzf_decompress.
 *
 * The buffers must not be overlapping.
 *
 * If the option LZF_STATE_ARG is enabled, an extra argument must be
 * supplied which is not reflected in this header file. Refer to lzfP.h
 * and lzf_c.c.
 *
 */
static unsigned int
lzf_compress(const void *const in_data, unsigned int in_len,
	      void *out_data, unsigned int out_len
#if LZF_STATE_ARG
              , LZF_STATE htab
#endif
              )
{
#if !LZF_STATE_ARG
  LZF_STATE htab;
#endif
  const u8 **hslot;
  const u8 *ip = (const u8 *)in_data;
        u8 *op = (u8 *)out_data;
  const u8 *in_end  = ip + in_len;
        u8 *out_end = op + out_len;
  const u8 *ref;

  /* off requires a type wide enough to hold a general pointer difference.
   * ISO C doesn't have that (size_t might not be enough and ptrdiff_t only
   * works for differences within a single object). We also assume that no
   * no bit pattern traps. Since the only platform that is both non-POSIX
   * and fails to support both assumptions is windows 64 bit, we make a
   * special workaround for it.
   */
#if defined (WIN32) && defined (_M_X64)
  unsigned _int64 off; /* workaround for missing POSIX compliance */
#else
  unsigned long off;
#endif
  unsigned int hval;
  int lit;

  if (!in_len || !out_len)
    return 0;

#if INIT_HTAB
  memset (htab, 0, sizeof (htab));
# if 0
  for (hslot = htab; hslot < htab + HSIZE; hslot++)
    *hslot++ = ip;
# endif
#endif

  lit = 0; op++; /* start run */

  hval = FRST (ip);
  while (ip < in_end - 2)
    {
      hval = NEXT (hval, ip);
      hslot = htab + IDX (hval);
      ref = *hslot; *hslot = ip;

      if (1
#if INIT_HTAB
          && ref < ip /* the next test will actually take care of this, but this is faster */
#endif
          && (off = ip - ref - 1) < MAX_OFF
          && ip + 4 < in_end
          && ref > (u8 *)in_data
#if STRICT_ALIGN
          && ref[0] == ip[0]
          && ref[1] == ip[1]
          && ref[2] == ip[2]
#else
          && *(u16 *)ref == *(u16 *)ip
          && ref[2] == ip[2]
#endif
        )
        {
          /* match found at *ref++ */
          unsigned int len = 2;
          unsigned int maxlen = in_end - ip - len;
          maxlen = maxlen > MAX_REF ? MAX_REF : maxlen;

          if (expect_false (op + 3 + 1 >= out_end)) /* first a faster conservative test */
            if (op - !lit + 3 + 1 >= out_end) /* second the exact but rare test */
              return 0;

          op [- lit - 1] = lit - 1; /* stop run */
          op -= !lit; /* undo run if length is zero */

          for (;;)
            {
              if (expect_true (maxlen > 16))
                {
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;

                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;

                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;

                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                  len++; if (ref [len] != ip [len]) break;
                }

              do
                len++;
              while (len < maxlen && ref[len] == ip[len]);

              break;
            }

          len -= 2; /* len is now #octets - 1 */
          ip++;

          if (len < 7)
            {
              *op++ = (off >> 8) + (len << 5);
            }
          else
            {
              *op++ = (off >> 8) + (  7 << 5);
              *op++ = len - 7;
            }

          *op++ = off;
          lit = 0; op++; /* start run */

          ip += len + 1;

          if (expect_false (ip >= in_end - 2))
            break;

#if ULTRA_FAST || VERY_FAST
          --ip;
# if VERY_FAST && !ULTRA_FAST
          --ip;
# endif
          hval = FRST (ip);

          hval = NEXT (hval, ip);
          htab[IDX (hval)] = ip;
          ip++;

# if VERY_FAST && !ULTRA_FAST
          hval = NEXT (hval, ip);
          htab[IDX (hval)] = ip;
          ip++;
# endif
#else
          ip -= len + 1;

          do
            {
              hval = NEXT (hval, ip);
              htab[IDX (hval)] = ip;
              ip++;
            }
          while (len--);
#endif
        }
      else
        {
          /* one more literal byte we must copy */
          if (expect_false (op >= out_end))
            return 0;

          lit++; *op++ = *ip++;

          if (expect_false (lit == MAX_LIT))
            {
              op [- lit - 1] = lit - 1; /* stop run */
              lit = 0; op++; /* start run */
            }
        }
    }

  if (op + 3 > out_end) /* at most 3 bytes can be missing here */
    return 0;

  while (ip < in_end)
    {
      lit++; *op++ = *ip++;

      if (expect_false (lit == MAX_LIT))
        {
          op [- lit - 1] = lit - 1; /* stop run */
          lit = 0; op++; /* start run */
        }
    }

  op [- lit - 1] = lit - 1; /* end run */
  op -= !lit; /* undo run if length is zero */

  return op - (u8 *)out_data;
}

/*
 * Decompress data compressed with some version of the lzf_compress
 * function and stored at location in_data and length in_len. The result
 * will be stored at out_data up to a maximum of out_len characters.
 *
 * If the output buffer is not large enough to hold the decompressed
 * data, -E2BIG is returned. Otherwise the number
 * of decompressed bytes (i.e. the original length of the data) is
 * returned.
 *
 * If an error in the compressed data is detected, -EINVAL is returned.
 *
 * This function is very fast, about as fast as a copying loop.
 */
static int 
lzf_decompress (const void *const in_data,  unsigned int in_len,
                void             *out_data, unsigned int out_len)
{
  u8 const *ip = (const u8 *)in_data;
  u8       *op = (u8 *)out_data;
  u8 const *const in_end  = ip + in_len;
  u8       *const out_end = op + out_len;

  do
    {
      unsigned int ctrl = *ip++;

      if (ctrl < (1 << 5)) /* literal run */
        {
          ctrl++;

          if (op + ctrl > out_end)
            {
              return -E2BIG;
            }

#if CHECK_INPUT
          if (ip + ctrl > in_end)
            {
              return -EINVAL;
            }
#endif

#ifdef lzf_movsb
          lzf_movsb (op, ip, ctrl);
#else
          do
            *op++ = *ip++;
          while (--ctrl);
#endif
        }
      else /* back reference */
        {
          unsigned int len = ctrl >> 5;

          u8 *ref = op - ((ctrl & 0x1f) << 8) - 1;

#if CHECK_INPUT
          if (ip >= in_end)
            {
              return -EINVAL;
            }
#endif
          if (len == 7)
            {
              len += *ip++;
#if CHECK_INPUT
              if (ip >= in_end)
                {
                  return -EINVAL;
                }
#endif
            }

          ref -= *ip++;

          if (op + len + 2 > out_end)
            {
              return -E2BIG;
            }

          if (ref < (u8 *)out_data)
            {
              return -EINVAL;
            }

#ifdef lzf_movsb
          len += 2;
          lzf_movsb (op, ref, len);
#else
          *op++ = *ref++;
          *op++ = *ref++;

          do
            *op++ = *ref++;
          while (--len);
#endif
        }
    }
  while (ip < in_end);

  return op - (u8 *)out_data;
}


/* Wrapper code starts here.  For ease of updating to newer versions of
   liblzf, the above code is modified as little as possible from the
   upstream version. */

static enum isrcry_result lzf_alloc(struct isrcry_compress_ctx *cctx)
{
	if (cctx->level != 0)
		return ISRCRY_INVALID_ARGUMENT;
	cctx->ctx = g_slice_new(LZF_STATE);
	return ISRCRY_OK;
}

static void lzf_free(struct isrcry_compress_ctx *cctx)
{
	g_slice_free(LZF_STATE, cctx->ctx);
}

static enum isrcry_result lzf_do_compress(struct isrcry_compress_ctx *cctx,
			const unsigned char *in, unsigned *inlen,
			unsigned char *out, unsigned *outlen)
{
	unsigned result;
	enum isrcry_result ret = ISRCRY_OK;

	if (*inlen == 0) {
		/* lzf_compress() returns 0 on error, which almost always
		   means that the buffer is too small.  Separately detect
		   the one case where it means something else. */
		ret = ISRCRY_INVALID_ARGUMENT;
	} else {
		result = lzf_compress(in, *inlen, out, *outlen, cctx->ctx);
		if (result)
			*outlen = result;
		else
			ret = ISRCRY_BUFFER_OVERFLOW;
	}
	if (ret) {
		*inlen = 0;
		*outlen = 0;
		return ret;
	}
	return ISRCRY_OK;
}

static enum isrcry_result lzf_do_decompress(struct isrcry_compress_ctx *cctx,
			const unsigned char *in, unsigned *inlen,
			unsigned char *out, unsigned *outlen)
{
	int result;

	(void)cctx;  /* silence compiler warning */

	result = lzf_decompress(in, *inlen, out, *outlen);
	if (result < 0) {
		*inlen = 0;
		*outlen = 0;
		if (result == -E2BIG)
			return ISRCRY_BUFFER_OVERFLOW;
		else if (result == -EINVAL)
			return ISRCRY_BAD_FORMAT;
		else
			g_assert_not_reached();
	}
	*outlen = result;
	return ISRCRY_OK;
}

const struct isrcry_compress_desc _isrcry_lzf_desc = {
	.can_stream = FALSE,
	.alloc = lzf_alloc,
	.free = lzf_free,
	.compress_final = lzf_do_compress,
	.decompress_final = lzf_do_decompress
};
