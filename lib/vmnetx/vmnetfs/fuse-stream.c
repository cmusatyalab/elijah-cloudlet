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

#include <errno.h>
#include "vmnetfs-private.h"

static int stream_getattr(void *dentry_ctx G_GNUC_UNUSED, struct stat *st)
{
    st->st_mode = S_IFREG | 0400;
    return 0;
}

static int stream_open(void *dentry_ctx, struct vmnetfs_fuse_fh *fh)
{
    struct vmnetfs_stream_group *sgrp = dentry_ctx;

    fh->data = _vmnetfs_stream_new(sgrp);
    return 0;
}

static int stream_read(struct vmnetfs_fuse_fh *fh, void *buf,
        uint64_t start G_GNUC_UNUSED, uint64_t count)
{
    struct vmnetfs_stream *strm = fh->data;
    GError *err = NULL;
    int ret;

    ret = _vmnetfs_stream_read(strm, buf, count, fh->blocking, &err);
    if (err) {
        if (g_error_matches(err, VMNETFS_STREAM_ERROR,
                VMNETFS_STREAM_ERROR_NONBLOCKING)) {
            ret = -EAGAIN;
        } else if (g_error_matches(err, VMNETFS_IO_ERROR,
                VMNETFS_IO_ERROR_INTERRUPTED)) {
            ret = -EINTR;
        } else if (g_error_matches(err, VMNETFS_STREAM_ERROR,
                VMNETFS_STREAM_ERROR_CLOSED)) {
            ret = 0;
        } else {
            ret = -EIO;
        }
        g_clear_error(&err);
    }
    return ret;
}

static int stream_poll(struct vmnetfs_fuse_fh *fh, struct fuse_pollhandle *ph,
        bool *readable)
{
    struct vmnetfs_stream *strm = fh->data;

    *readable = _vmnetfs_stream_add_poll_handle(strm, ph);
    return 0;
}

static void stream_release(struct vmnetfs_fuse_fh *fh)
{
    struct vmnetfs_stream *strm = fh->data;

    _vmnetfs_stream_free(strm);
}

static const struct vmnetfs_fuse_ops stream_ops = {
    .getattr = stream_getattr,
    .open = stream_open,
    .read = stream_read,
    .poll = stream_poll,
    .release = stream_release,
    .nonseekable = true,
};

void _vmnetfs_fuse_stream_populate(struct vmnetfs_fuse_dentry *dir,
        struct vmnetfs_image *img)
{
    struct vmnetfs_fuse_dentry *streams;

    streams = _vmnetfs_fuse_add_dir(dir, "streams");
    _vmnetfs_fuse_add_file(streams, "chunks_accessed", &stream_ops,
            _vmnetfs_bit_get_stream_group(img->accessed_map));
    _vmnetfs_fuse_add_file(streams, "chunks_base", &stream_ops,
            _vmnetfs_bit_get_stream_group(img->total_overlay_map));
    _vmnetfs_fuse_add_file(streams, "chunks_overlay", &stream_ops,
                _vmnetfs_bit_get_stream_group(img->current_overlay_map));
    _vmnetfs_fuse_add_file(streams, "chunks_modified", &stream_ops,
            _vmnetfs_bit_get_stream_group(img->modified_map));
    _vmnetfs_fuse_add_file(streams, "io", &stream_ops, img->io_stream);
}
