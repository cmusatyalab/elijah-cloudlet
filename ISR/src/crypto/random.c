/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * Copyright (C) 2008-2010 Carnegie Mellon University
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

/* We used to have a full PRNG here.  However, ISR only uses a small amount
   of randomness, infrequently, in a few places, so that seemed like overkill.
   The kernel's RNG has more entropy to draw on, from a longer-lived entropy
   pool, and sees wider testing and use, so we now use that instead.  Doing so
   is probably slower than using our own PRNG, but for our limited uses
   that's fine. */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "isrcrypto.h"
#define LIBISRCRYPTO_INTERNAL
#include "internal.h"

#define RANDOM_DEVICE "/dev/urandom"

struct isrcry_random_ctx {
	int fd;
};

exported struct isrcry_random_ctx *isrcry_random_alloc(void)
{
	struct isrcry_random_ctx *rctx;
	int fd;

	fd = open(RANDOM_DEVICE, O_RDONLY);
	if (fd == -1)
		return NULL;
	rctx = g_slice_new0(struct isrcry_random_ctx);
	rctx->fd = fd;
	return rctx;
}

exported void isrcry_random_bytes(struct isrcry_random_ctx *rctx, void *buffer,
	                   unsigned length)
{
	ssize_t rcount;

	while (length > 0) {
		rcount = read(rctx->fd, buffer, length);
		if (rcount > 0) {
			buffer += rcount;
			length -= rcount;
		}
	}
}

exported void isrcry_random_free(struct isrcry_random_ctx *rctx)
{
	close(rctx->fd);
	g_slice_free(struct isrcry_random_ctx, rctx);
}
