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

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "vmnetfs-private.h"

/* An item that may change over time. */
struct vmnetfs_pollable {
    GMutex *lock;
    GList *unchanged;
    uint64_t generation;
};

struct vmnetfs_pollable *_vmnetfs_pollable_new(void)
{
    struct vmnetfs_pollable *pll;

    pll = g_slice_new0(struct vmnetfs_pollable);
    pll->lock = g_mutex_new();
    return pll;
}

uint64_t _vmnetfs_pollable_get_change_cookie(struct vmnetfs_pollable *pll)
{
    uint64_t change_cookie;

    g_mutex_lock(pll->lock);
    change_cookie = pll->generation;
    g_mutex_unlock(pll->lock);
    return change_cookie;
}

/* lock must be held */
static void add_handle(struct vmnetfs_pollable *pll,
        struct fuse_pollhandle *ph, bool changed)
{
    if (ph == NULL) {
        return;
    }
    if (changed) {
        fuse_notify_poll(ph);
        fuse_pollhandle_destroy(ph);
    } else {
        pll->unchanged = g_list_prepend(pll->unchanged, ph);
    }
}

/* If @changed, notifies the fuse_pollhandle, otherwise queues it. */
void _vmnetfs_pollable_add_poll_handle(struct vmnetfs_pollable *pll,
        struct fuse_pollhandle *ph, bool changed)
{
    g_mutex_lock(pll->lock);
    add_handle(pll, ph, changed);
    g_mutex_unlock(pll->lock);
}

/* Returns true and notifies the fuse_pollhandle if the pollable has
   changed since change_cookie was set, otherwise queues the fuse_pollhandle
   and returns false.  @ph may be null if the caller only wants the return
   value. */
bool _vmnetfs_pollable_add_poll_handle_conditional(
        struct vmnetfs_pollable *pll, struct fuse_pollhandle *ph,
        uint64_t change_cookie)
{
    bool changed;

    g_mutex_lock(pll->lock);
    changed = pll->generation != change_cookie;
    add_handle(pll, ph, changed);
    g_mutex_unlock(pll->lock);
    return changed;
}

/* lock must be held */
static void release_handles(struct vmnetfs_pollable *pll, bool notify)
{
    struct fuse_pollhandle *ph;
    GList *el;

    for (el = g_list_first(pll->unchanged); el != NULL; el = g_list_next(el)) {
        ph = el->data;
        if (notify) {
            fuse_notify_poll(ph);
        }
        fuse_pollhandle_destroy(ph);
    }
    g_list_free(pll->unchanged);
    pll->unchanged = NULL;
}

void _vmnetfs_pollable_change(struct vmnetfs_pollable *pll)
{
    g_mutex_lock(pll->lock);
    pll->generation++;
    release_handles(pll, true);
    g_mutex_unlock(pll->lock);
}

void _vmnetfs_pollable_free(struct vmnetfs_pollable *pll)
{
    if (pll == NULL) {
        return;
    }
    g_mutex_lock(pll->lock);
    release_handles(pll, false);
    g_mutex_unlock(pll->lock);
    g_mutex_free(pll->lock);
    g_slice_free(struct vmnetfs_pollable, pll);
}
