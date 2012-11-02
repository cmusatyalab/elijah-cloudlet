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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "vmnetfs-private.h"

//#define DEBUG_LL
#ifdef DEBUG_LL
#define DPRINTF(fmt, ...) \
    do { fprintf(stdout, "[DEBUG] IO: " fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

bool _vmnetfs_ll_modified_init(struct vmnetfs_image *img, GError **err)
{
    char *file;

    file = g_strdup_printf("%s/vmnetfs-XXXXXX", g_get_tmp_dir());
    img->write_fd = mkstemp(file);
    if (img->write_fd == -1) {
        g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Couldn't create temporary file: %s", strerror(errno));
        g_free(file);
        return false;
    }
    unlink(file);
    g_free(file);
    /* set_on_extend ensures that chunks that are truncated away are not
       retrieved from the pristine cache if the image is extended again. */
    img->modified_map = _vmnetfs_bit_new(img->bitmaps, true);
    return true;
}

void _vmnetfs_ll_modified_destroy(struct vmnetfs_image *img)
{
    _vmnetfs_bit_free(img->modified_map);
    close(img->write_fd);
}

bool _vmnetfs_ll_modified_read_chunk(struct vmnetfs_image *img,
        uint64_t image_size, void *data, uint64_t chunk, uint32_t offset,
        uint32_t length, GError **err)
{
    g_assert(_vmnetfs_bit_test(img->modified_map, chunk));
    g_assert(offset < img->chunk_size);
    g_assert(offset + length <= img->chunk_size);
    g_assert(chunk * img->chunk_size + offset + length <= image_size);

    return _vmnetfs_safe_pread("image", img->write_fd, data, length,
            chunk * img->chunk_size + offset, err);
}

bool _vmnetfs_ll_modified_write_chunk(struct vmnetfs_image *img,
        uint64_t image_size, const void *data, uint64_t chunk,
        uint32_t offset, uint32_t length, GError **err)
{
	DPRINTF("krha, _vmnetfs_ll_modified_write_chunk, start(%ld), length(%ld)\n", \
			chunk * img->chunk_size + offset, length);
    g_assert(_vmnetfs_bit_test(img->modified_map, chunk) ||
            (offset == 0 && length == MIN(img->chunk_size,
            img->initial_size - chunk * img->chunk_size)));
    g_assert(offset < img->chunk_size);
    g_assert(offset + length <= img->chunk_size);
    g_assert(chunk * img->chunk_size + offset + length <= image_size);

    if (_vmnetfs_safe_pwrite("image", img->write_fd, data, length,
            chunk * img->chunk_size + offset, err)) {
    	// every modification should be recorded
    	// _vmnetfs_bit_set send stream only when it is new bit map change
        _vmnetfs_bit_set_force(img->modified_map, chunk, true);
        return true;
    } else {
        return false;
    }
}

bool _vmnetfs_ll_modified_set_size(struct vmnetfs_image *img,
        uint64_t current_size, uint64_t new_size, GError **err)
{
    /* If we're truncating the new last chunk, it must be in the modified
       cache to ensure that subsequent expansions don't reveal the truncated
       part. */
    g_assert(new_size > current_size ||
            new_size % img->chunk_size == 0 ||
            _vmnetfs_bit_test(img->modified_map, new_size / img->chunk_size));

    if (ftruncate(img->write_fd, new_size)) {
        g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Couldn't truncate image: %s", strerror(errno));
        return false;
    }

    return true;
}
