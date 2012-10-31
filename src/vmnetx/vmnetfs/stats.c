/*
 * vmnetfs - virtual machine network execution virtual filesystem
 *
 * Copyright (C) 2006-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * COPYING.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "vmnetfs-private.h"

struct vmnetfs_stat {
    GMutex *lock;
    struct vmnetfs_pollable *pll;
    bool closed;
    uint64_t u64;
};

struct vmnetfs_stat *_vmnetfs_stat_new(void)
{
    struct vmnetfs_stat *stat;

    stat = g_slice_new0(struct vmnetfs_stat);
    stat->lock = g_mutex_new();
    stat->pll = _vmnetfs_pollable_new();
    return stat;
}

void _vmnetfs_stat_close(struct vmnetfs_stat *stat)
{
    g_mutex_lock(stat->lock);
    stat->closed = true;
    _vmnetfs_pollable_change(stat->pll);
    g_mutex_unlock(stat->lock);
}

bool _vmnetfs_stat_is_closed(struct vmnetfs_stat *stat)
{
    bool ret;

    g_mutex_lock(stat->lock);
    ret = stat->closed;
    g_mutex_unlock(stat->lock);
    return ret;
}

void _vmnetfs_stat_free(struct vmnetfs_stat *stat)
{
    if (stat == NULL) {
        return;
    }
    _vmnetfs_pollable_free(stat->pll);
    g_mutex_free(stat->lock);
    g_slice_free(struct vmnetfs_stat, stat);
}

bool _vmnetfs_stat_add_poll_handle(struct vmnetfs_stat *stat,
        struct fuse_pollhandle *ph, uint64_t change_cookie)
{
    bool ret;

    g_mutex_lock(stat->lock);
    if (stat->closed) {
        _vmnetfs_pollable_add_poll_handle(stat->pll, ph, true);
        ret = true;
    } else {
        ret = _vmnetfs_pollable_add_poll_handle_conditional(stat->pll, ph,
                change_cookie);
    }
    g_mutex_unlock(stat->lock);
    return ret;
}

void _vmnetfs_u64_stat_increment(struct vmnetfs_stat *stat, uint64_t val)
{
    g_mutex_lock(stat->lock);
    stat->u64 += val;
    _vmnetfs_pollable_change(stat->pll);
    g_mutex_unlock(stat->lock);
}

uint64_t _vmnetfs_u64_stat_get(struct vmnetfs_stat *stat,
        uint64_t *change_cookie)
{
    uint64_t ret;

    g_mutex_lock(stat->lock);
    ret = stat->u64;
    if (change_cookie != NULL) {
        *change_cookie = _vmnetfs_pollable_get_change_cookie(stat->pll);
    }
    g_mutex_unlock(stat->lock);
    return ret;
}
