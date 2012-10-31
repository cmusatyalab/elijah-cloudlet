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
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include "vmnetfs-private.h"

/*
#define CHUNKS_PER_DIR 4096

static bool mkdir_with_parents(const char *dir, GError **err)
{
    if (g_mkdir_with_parents(dir, 0700)) {
        g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Couldn't create %s: %s", dir, strerror(errno));
        return false;
    }
    return true;
}

static uint64_t get_dir_num(uint64_t chunk)
{
    return chunk / CHUNKS_PER_DIR * CHUNKS_PER_DIR;
}

static char *get_dir(struct vmnetfs_image *img, uint64_t chunk)
{
    return g_strdup_printf("%s/%"PRIu64, img->read_base, get_dir_num(chunk));
}

static char *get_file(struct vmnetfs_image *img, uint64_t chunk)
{
    return g_strdup_printf("%s/%"PRIu64"/%"PRIu64, img->read_base,
            get_dir_num(chunk), chunk);
}
*/

/*
static bool set_present_from_directory(struct vmnetfs_image *img,
        const char *path, uint64_t dir_num, GError **err)
{
    GDir *dir;
    const char *file;
    uint64_t chunk;
    uint64_t chunks;
    char *endptr;

    chunks = (img->initial_size + img->chunk_size - 1) / img->chunk_size;
    dir = g_dir_open(path, 0, err);
    if (dir == NULL) {
        return false;
    }
    while ((file = g_dir_read_name(dir)) != NULL) {
        chunk = g_ascii_strtoull(file, &endptr, 10);
        if (*file == 0 || *endptr != 0 || chunk > chunks ||
                dir_num != get_dir_num(chunk)) {
            g_set_error(err, VMNETFS_IO_ERROR, VMNETFS_IO_ERROR_INVALID_CACHE,
                    "Invalid cache entry %s/%s", path, file);
            g_dir_close(dir);
            return false;
        }
        _vmnetfs_bit_set(img->total_overlay_map, chunk);
    }
    g_dir_close(dir);
    return true;
}
*/

bool _vmnetfs_ll_pristine_init(struct vmnetfs_image *img, GError **err)
{
    gchar **components;
    gchar **cur;
    gchar *end;
    u_int64_t chunk_number = 0;
    u_int valid_bit = 0;

    // initialize total_overlay_map with total overlay information
    img->total_overlay_map = _vmnetfs_bit_new(img->bitmaps, false);
    img->current_overlay_map = _vmnetfs_bit_new(img->bitmaps, false);

    components = g_strsplit(img->total_overlay_chunks, ",", 0);
    for (cur = components; *cur != NULL; cur++) {
    	gchar **overlay_info = g_strsplit(*cur, ":", 0);
    	chunk_number = g_ascii_strtoull(*overlay_info, &end, 10);
    	if (*overlay_info == end){
            g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
                    "Invalid overlay format at chunk number %s", *overlay_info);
    	}
    	valid_bit = (int)g_ascii_strtoull(*(overlay_info+1), &end, 10);
    	if (*overlay_info == end){
    		g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
    				"Invalid overlay format at valid bit %s", *(overlay_info+1));
    	}

    	// Set bit for total_overlay_map
        _vmnetfs_bit_set(img->total_overlay_map, chunk_number);
        if(valid_bit == 1){
        	// Set bit for current_overlay_map only when the chunk is available
            _vmnetfs_bit_set(img->current_overlay_map, chunk_number);
        }
    	g_strfreev(overlay_info);
    }
    g_strfreev(components);

    return true;
}

void _vmnetfs_ll_pristine_destroy(struct vmnetfs_image *img)
{
    _vmnetfs_bit_free(img->total_overlay_map);
    _vmnetfs_bit_free(img->current_overlay_map);
}

/*
bool _vmnetfs_ll_pristine_read_chunk(struct vmnetfs_image *img, void *data,
        uint64_t chunk, uint32_t offset, uint32_t length, GError **err)
{
    char *file;
    int fd;
    bool ret;

    g_assert(_vmnetfs_bit_test(img->base_map, chunk));
    g_assert(offset < img->chunk_size);
    g_assert(offset + length <= img->chunk_size);
    g_assert(chunk * img->chunk_size + offset + length <= img->initial_size);

    file = get_file(img, chunk);
    fd = open(file, O_RDONLY);
    if (fd == -1) {
        g_set_error(err, G_FILE_ERROR, g_file_error_from_errno(errno),
                "Couldn't open %s: %s", file, strerror(errno));
        g_free(file);
        return false;
    }
    ret = _vmnetfs_safe_pread(file, fd, data, length, offset, err);
    close(fd);
    g_free(file);
    return ret;
}
*/

/*
bool _vmnetfs_ll_pristine_write_chunk(struct vmnetfs_image *img, void *data,
        uint64_t chunk, uint32_t length, GError **err)
{
    char *dir;
    char *file;
    bool ret;

    g_assert(length <= img->chunk_size);
    g_assert(chunk * img->chunk_size + length <= img->initial_size);

    dir = get_dir(img, chunk);
    file = get_file(img, chunk);

    ret = mkdir_with_parents(dir, err);
    if (!ret) {
        goto out;
    }
    ret = g_file_set_contents(file, data, length, err);
    if (!ret) {
        goto out;
    }
    _vmnetfs_bit_set(img->base_map, chunk);

out:
    g_free(file);
    g_free(dir);
    return ret;
}
*/

bool _cloudlet_read_chunk(struct vmnetfs_image *img,
		struct bitmap *bit_map, int read_fd, void *data,
		uint64_t chunk, uint32_t offset, uint32_t length, GError **err)
{
    bool ret;
    ret = _vmnetfs_safe_pread("cloudlet", read_fd, data, length, chunk * img->chunk_size + offset, err);
    return ret;
}

