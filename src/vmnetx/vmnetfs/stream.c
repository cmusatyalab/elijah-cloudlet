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

#include <stdarg.h>
#include <string.h>
#include "vmnetfs-private.h"

#define BLOCK_SIZE 8192

struct vmnetfs_stream_group {
    GMutex *lock;
    GList *streams;
    populate_stream_fn *populate;
    void *populate_data;
    bool closed;
};

struct vmnetfs_stream {
    struct vmnetfs_stream_group *group;
    GList *group_link;

    GMutex *lock;
    struct vmnetfs_cond *cond;
    GQueue *blocks;
    uint32_t head_read_offset;
    uint32_t tail_write_offset;
    struct vmnetfs_pollable *pll;
    bool closed;
};

static void *block_new(void)
{
    return g_slice_alloc(BLOCK_SIZE);
}

static void block_free(void *block)
{
    g_slice_free1(BLOCK_SIZE, block);
}

/* Notify waiters of changes to the stream.  Stream lock must be held. */
static void notify_stream(struct vmnetfs_stream *strm)
{
    _vmnetfs_cond_broadcast(strm->cond);
    _vmnetfs_pollable_change(strm->pll);
}

struct vmnetfs_stream_group *_vmnetfs_stream_group_new(
        populate_stream_fn *populate, void *populate_data)
{
    struct vmnetfs_stream_group *sgrp;

    sgrp = g_slice_new0(struct vmnetfs_stream_group);
    sgrp->lock = g_mutex_new();
    sgrp->populate = populate;
    sgrp->populate_data = populate_data;
    return sgrp;
}

static void close_stream(struct vmnetfs_stream *strm,
        void *user_data G_GNUC_UNUSED)
{
    g_mutex_lock(strm->lock);
    strm->closed = true;
    notify_stream(strm);
    g_mutex_unlock(strm->lock);
}

/* Cause streams in the stream group to return end-of-file rather than
   VMNETFS_STREAM_ERROR_NONBLOCKING or blocking.  This tells stream readers
   to close their file descriptors so the filesystem can be unmounted. */
void _vmnetfs_stream_group_close(struct vmnetfs_stream_group *sgrp)
{
    g_mutex_lock(sgrp->lock);
    if (!sgrp->closed) {
        sgrp->closed = true;
        g_list_foreach(sgrp->streams, (GFunc) close_stream, NULL);
    }
    g_mutex_unlock(sgrp->lock);
}

void _vmnetfs_stream_group_free(struct vmnetfs_stream_group *sgrp)
{
    g_assert(g_list_length(sgrp->streams) == 0);
    g_mutex_free(sgrp->lock);
    g_slice_free(struct vmnetfs_stream_group, sgrp);
}

struct vmnetfs_stream *_vmnetfs_stream_new(struct vmnetfs_stream_group *sgrp)
{
    struct vmnetfs_stream *strm;

    strm = g_slice_new0(struct vmnetfs_stream);
    strm->lock = g_mutex_new();
    strm->cond = _vmnetfs_cond_new();
    strm->blocks = g_queue_new();
    strm->tail_write_offset = BLOCK_SIZE;
    strm->pll = _vmnetfs_pollable_new();
    if (sgrp->populate != NULL) {
        sgrp->populate(strm, sgrp->populate_data);
    }

    g_mutex_lock(sgrp->lock);
    sgrp->streams = g_list_prepend(sgrp->streams, strm);
    strm->group = sgrp;
    strm->group_link = sgrp->streams;
    strm->closed = sgrp->closed;
    g_mutex_unlock(sgrp->lock);

    return strm;
}

void _vmnetfs_stream_free(struct vmnetfs_stream *strm)
{
    void *block;

    g_mutex_lock(strm->group->lock);
    strm->group->streams = g_list_delete_link(strm->group->streams,
            strm->group_link);
    g_mutex_unlock(strm->group->lock);

    _vmnetfs_pollable_free(strm->pll);
    while ((block = g_queue_pop_head(strm->blocks)) != NULL) {
        block_free(block);
    }
    g_queue_free(strm->blocks);
    _vmnetfs_cond_free(strm->cond);
    g_mutex_free(strm->lock);
    g_slice_free(struct vmnetfs_stream, strm);
}

uint64_t _vmnetfs_stream_read(struct vmnetfs_stream *strm, void *buf,
        uint64_t count, bool blocking, GError **err)
{
    void *block;
    uint64_t block_length;
    uint64_t cur;
    uint64_t copied = 0;

    g_mutex_lock(strm->lock);
    while (copied < count) {
        g_assert(strm->head_read_offset < BLOCK_SIZE);
        block = g_queue_peek_head(strm->blocks);
        if (block == NULL) {
            /* No more data at the moment. */
            if (copied > 0) {
                break;
            } else if (strm->closed) {
                g_set_error(err, VMNETFS_STREAM_ERROR,
                        VMNETFS_STREAM_ERROR_CLOSED, "Stream closed");
                break;
            } else if (blocking) {
                if (_vmnetfs_cond_wait(strm->cond, strm->lock)) {
                    g_set_error(err, VMNETFS_IO_ERROR,
                            VMNETFS_IO_ERROR_INTERRUPTED,
                            "Operation interrupted");
                    break;
                } else {
                    continue;
                }
            } else {
                g_set_error(err, VMNETFS_STREAM_ERROR,
                        VMNETFS_STREAM_ERROR_NONBLOCKING,
                        "No input available");
                break;
            }
        }
        if (g_queue_peek_tail(strm->blocks) == block) {
            block_length = strm->tail_write_offset;
        } else {
            block_length = BLOCK_SIZE;
        }
        cur = MIN(count - copied, block_length - strm->head_read_offset);
        memcpy(buf + copied, block + strm->head_read_offset, cur);
        copied += cur;
        strm->head_read_offset += cur;
        if (strm->head_read_offset == BLOCK_SIZE) {
            /* Finished a complete block */
            block_free(g_queue_pop_head(strm->blocks));
            strm->head_read_offset = 0;
        } else if (g_queue_peek_tail(strm->blocks) == block &&
                strm->head_read_offset == strm->tail_write_offset) {
            /* We're in the middle of a partial block but
               we've consumed all data.  Delete the block. */
            block_free(g_queue_pop_head(strm->blocks));
            strm->head_read_offset = 0;
            strm->tail_write_offset = BLOCK_SIZE;
        }
    }
    g_mutex_unlock(strm->lock);
    return copied;
}

static void stream_write(struct vmnetfs_stream *strm, const void *buf,
        uint64_t count)
{
    void *block;
    uint64_t cur;
    uint64_t copied = 0;

    g_mutex_lock(strm->lock);
    while (copied < count) {
        g_assert(strm->tail_write_offset <= BLOCK_SIZE);
        if (strm->tail_write_offset == BLOCK_SIZE) {
            g_queue_push_tail(strm->blocks, block_new());
            strm->tail_write_offset = 0;
        }
        block = g_queue_peek_tail(strm->blocks);
        cur = MIN(count - copied, BLOCK_SIZE - strm->tail_write_offset);
        memcpy(block + strm->tail_write_offset, buf + copied, cur);
        copied += cur;
        strm->tail_write_offset += cur;
    }
    notify_stream(strm);
    g_mutex_unlock(strm->lock);
}

void _vmnetfs_stream_write(struct vmnetfs_stream *strm, const char *fmt, ...)
{
    char *buf;
    uint64_t len;
    va_list ap;

    va_start(ap, fmt);
    buf = g_strdup_vprintf(fmt, ap);
    len = strlen(buf);
    va_end(ap);

    stream_write(strm, buf, len);
    g_free(buf);
}

void _vmnetfs_stream_group_write(struct vmnetfs_stream_group *sgrp,
        const char *fmt, ...)
{
    GList *el;
    char *buf;
    uint64_t len;
    va_list ap;

    va_start(ap, fmt);
    buf = g_strdup_vprintf(fmt, ap);
    len = strlen(buf);
    va_end(ap);

    g_mutex_lock(sgrp->lock);
    for (el = g_list_first(sgrp->streams); el != NULL; el = g_list_next(el)) {
        stream_write(el->data, buf, len);
    }
    g_mutex_unlock(sgrp->lock);
    g_free(buf);
}

bool _vmnetfs_stream_add_poll_handle(struct vmnetfs_stream *strm,
        struct fuse_pollhandle *ph)
{
    bool readable;

    g_mutex_lock(strm->lock);
    readable = g_queue_peek_head(strm->blocks) != NULL || strm->closed;
    _vmnetfs_pollable_add_poll_handle(strm->pll, ph, readable);
    g_mutex_unlock(strm->lock);
    return readable;
}
