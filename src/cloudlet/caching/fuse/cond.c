/*
 * cloudletcachefs - cloudlet cachcing emulation fs
 *
 * copyright (c) 2011-2013 carnegie mellon university
 *
 * this program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the gnu general public license as published
 * by the free software foundation.  a copy of the gnu general public license
 * should have been distributed along with this program in the file
 * copying.
 *
 * this program is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  see the gnu general public license
 * for more details.
 */

#include <signal.h>
#include <pthread.h>
#include "cachefs-private.h"

struct cachefs_cond *_cachefs_cond_new(void)
{
    struct cachefs_cond *cond;

    cond = g_slice_new0(struct cachefs_cond);
    cond->lock = g_mutex_new();
    cond->condition = g_cond_new();
    return cond;
}

void _cachefs_cond_free(struct cachefs_cond *cond)
{
    g_mutex_free(cond->lock);
    g_cond_free(cond->condition);
    g_slice_free(struct cachefs_cond, cond);
}

void _cachefs_cond_broadcast(struct cachefs_cond *cond)
{
    g_mutex_lock(cond->lock);
    _cachefs_write_debug("[cond] before broadcasting %x", cond->condition);
    g_cond_broadcast(cond->condition);
	//_cachefs_write_debug("[cond] after broadcasting %x", cond->condition);
    g_mutex_unlock(cond->lock);
}
