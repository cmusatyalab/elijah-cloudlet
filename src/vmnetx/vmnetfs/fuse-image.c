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

#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "vmnetfs-private.h"

struct io_cursor {
    /* Public fields; do not modify */
    uint64_t chunk;
    uint64_t offset;
    uint64_t length;
    uint64_t buf_offset;

    /* Private fields */
    struct vmnetfs_image *img;
    uint64_t start;
    uint64_t count;
};

/* The cursor is assumed to be allocated on the stack; this just fills
   it in. */
static void io_start(struct vmnetfs_image *img, struct io_cursor *cur,
        uint64_t start, uint64_t count)
{
    memset(cur, 0, sizeof(*cur));
    cur->img = img;
    cur->start = start;
    cur->count = count;
}

/* Populate the public fields of the cursor with information on the next
   chunk in the I/O, starting from the first, given that the last I/O
   completed @count bytes.  Returns true if we produced a valid chunk,
   false if done with this I/O.  Assumes an infinite-size image. */
static bool io_chunk(struct io_cursor *cur, uint64_t count)
{
    // krha
	// offset		: offset within a chunk
	// buf_offset	: offset within requested buffer
    // length		: choose min between
    // 	1) left bytes within chunk_size in case where requested size of bigger than chunk
    // 	2) left bytes in last chunk
    uint64_t position;

    cur->buf_offset += count;
    if (cur->buf_offset >= cur->count) {
        /* Done */
        return false;
    }
    position = cur->start + cur->buf_offset;
    cur->chunk = position / cur->img->chunk_size;
    cur->offset = position - cur->chunk * cur->img->chunk_size;
    cur->length = MIN(cur->img->chunk_size - cur->offset,
            cur->count - cur->buf_offset);

    return true;
}

static int image_getattr(void *dentry_ctx, struct stat *st)
{
    struct vmnetfs_image *img = dentry_ctx;

    st->st_mode = S_IFREG | 0600;
    st->st_size = _vmnetfs_io_get_image_size(img, NULL);
    return 0;
}

static int image_truncate(void *dentry_ctx, uint64_t size)
{
	printf("krha, image_truncate, copy whole image\n");
    struct vmnetfs_image *img = dentry_ctx;
    GError *err = NULL;

    if (!_vmnetfs_io_set_image_size(img, size, &err)) {
        if (g_error_matches(err, VMNETFS_IO_ERROR,
                VMNETFS_IO_ERROR_INTERRUPTED)) {
            g_clear_error(&err);
            return -EINTR;
        } else {
            g_warning("%s", err->message);
            g_clear_error(&err);
            return -EIO;
        }
    }
    return 0;
}

static int image_open(void *dentry_ctx, struct vmnetfs_fuse_fh *fh)
{
    struct vmnetfs_image *img = dentry_ctx;
    printf("krha, image_open\n");

    fh->data = img;
    return 0;
}

static int image_read(struct vmnetfs_fuse_fh *fh, void *buf, uint64_t start,
        uint64_t count)
{
    struct vmnetfs_image *img = fh->data;
    struct io_cursor cur;
    GError *err = NULL;
    uint64_t read = 0;

    _vmnetfs_stream_group_write(img->io_stream, "read %"PRIu64"+%"PRIu64"\n",
            start, count);
    for (io_start(img, &cur, start, count); io_chunk(&cur, read); ) {
        read = _vmnetfs_io_read_chunk(img, buf + cur.buf_offset, cur.chunk,
                cur.offset, cur.length, &err);
        if (err) {
            if (g_error_matches(err, VMNETFS_IO_ERROR,
                    VMNETFS_IO_ERROR_INTERRUPTED)) {
                g_clear_error(&err);
                return (int) (cur.buf_offset + read) ?: -EINTR;
            } else if (g_error_matches(err, VMNETFS_IO_ERROR,
                    VMNETFS_IO_ERROR_EOF)) {
                g_clear_error(&err);
                return cur.buf_offset + read;
            } else {
                g_warning("%s", err->message);
                g_clear_error(&err);
                return (int) (cur.buf_offset + read) ?: -EIO;
            }
        }
        _vmnetfs_u64_stat_increment(img->bytes_read, cur.length);
    }
    return cur.buf_offset;
}

static int image_write(struct vmnetfs_fuse_fh *fh, const void *buf,
        uint64_t start, uint64_t count)
{
    printf("krha, image_write: start(%ld), count(%ld)\n", start, count);
    struct vmnetfs_image *img = fh->data;
    struct io_cursor cur;
    GError *err = NULL;
    uint64_t written = 0;

    _vmnetfs_stream_group_write(img->io_stream, "write %"PRIu64"+%"PRIu64"\n",
            start, count);
    for (io_start(img, &cur, start, count); io_chunk(&cur, written); ) {
        written = _vmnetfs_io_write_chunk(img, buf + cur.buf_offset, cur.chunk,
                cur.offset, cur.length, &err);
        if (err) {
            if (g_error_matches(err, VMNETFS_IO_ERROR,
                    VMNETFS_IO_ERROR_INTERRUPTED)) {
                g_clear_error(&err);
                return (int) (cur.buf_offset + written) ?: -EINTR;
            } else {
                g_warning("%s", err->message);
                g_clear_error(&err);
                return (int) (cur.buf_offset + written) ?: -EIO;
            }
        }
        _vmnetfs_u64_stat_increment(img->bytes_written, cur.length);
    }
    return cur.buf_offset;
}

static const struct vmnetfs_fuse_ops image_ops = {
    .getattr = image_getattr,
    .truncate = image_truncate,
    .open = image_open,
    .read = image_read,
    .write = image_write,
};

void _vmnetfs_fuse_image_populate(struct vmnetfs_fuse_dentry *dir,
        struct vmnetfs_image *img)
{
    _vmnetfs_fuse_add_file(dir, "image", &image_ops, img);
}
