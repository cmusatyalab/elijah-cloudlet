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
#include <sys/time.h>
#include "vmnetfs-private.h"

/* Bitmap rules:
   1. All bitmaps in a group have the same size.
   2. Memory allocations for the actual bits are always a power of 2, and
      are never reduced.
   3. Bitmaps are initially zeroed.
   4. Bitmaps have a set_on_extend flag that governs the behavior of bits
      beyond the end of the bitmap.
   5. Allocated but unused bits in a bitmap (those between nbits and
      8 * allocated_bytes) are set to set_on_extend.
   6. If a bitmap is reduced and set_on_extend is true, bits in the
      eliminated area are set to 1.  Otherwise, the bits are left alone.
*/

/* A set of bitmaps, each with the same size. */
struct bitmap_group {
    GMutex *lock;
    GList *maps;
    uint64_t nbits;
    uint64_t allocated_bytes;
};

/* struct bitmap requires external serialization to ensure that the bits
   don't change while the caller requires them to be consistent. */
struct bitmap {
    struct bitmap_group *mgrp;
    uint8_t *bits;
    struct vmnetfs_stream_group *sgrp;
    bool set_on_extend;
};

static void populate_stream(struct vmnetfs_stream *strm, void *_map)
{
    struct bitmap *map = _map;
    uint64_t byte;
    uint8_t bit;

    g_mutex_lock(map->mgrp->lock);
    for (byte = 0; byte < (map->mgrp->nbits + 7) / 8; byte++) {
        if (map->bits[byte]) {
            for (bit = 0; bit < 8; bit++) {
                if (byte * 8 + bit >= map->mgrp->nbits) {
                    break;
                }
                if (map->bits[byte] & (1 << bit)) {
                    _vmnetfs_stream_write(strm, "%"PRIu64"\n",
                            byte * 8 + bit);
                }
            }
        }
    }
    g_mutex_unlock(map->mgrp->lock);
}

/* Return the proper allocation to hold the specified number of bits. */
static uint64_t allocation_for_bits(uint64_t bits)
{
    /* Round up to the next power of two */
    return 1 << g_bit_storage(((bits - 1) + 7) / 8);
}

static void set_bit(struct bitmap *map, uint64_t bit)
{
    g_assert(bit < map->mgrp->allocated_bytes * 8);
    map->bits[bit / 8] |= 1 << (bit % 8);
}

struct timeval tv;

static void notify_bit(struct bitmap *map, uint64_t bit)
{
	gettimeofday(&tv, NULL);
    _vmnetfs_stream_group_write(map->sgrp, "%d.%d\t%"PRIu64"\n", tv.tv_sec, tv.tv_usec, bit);
}

static bool test_bit(struct bitmap *map, uint64_t bit)
{
    return !!(map->bits[bit / 8] & (1 << (bit % 8)));
}

struct bitmap_group *_vmnetfs_bit_group_new(uint64_t initial_bits)
{
    struct bitmap_group *mgrp;

    mgrp = g_slice_new0(struct bitmap_group);
    mgrp->lock = g_mutex_new();
    mgrp->nbits = initial_bits;
    mgrp->allocated_bytes = allocation_for_bits(initial_bits);
    return mgrp;
}

void _vmnetfs_bit_group_free(struct bitmap_group *mgrp)
{
    g_assert(mgrp->maps == NULL);
    g_mutex_free(mgrp->lock);
    g_slice_free(struct bitmap_group, mgrp);
}

void _vmnetfs_bit_group_resize(struct bitmap_group *mgrp, uint64_t bits)
{
    struct bitmap *map;
    GList *el;
    uint64_t allocation = allocation_for_bits(bits);
    uint64_t n;

    g_mutex_lock(mgrp->lock);
    if (bits > mgrp->nbits) {
        /* Increase allocation if necessary */
        if (allocation > mgrp->allocated_bytes) {
            for (el = g_list_first(mgrp->maps); el != NULL;
                    el = g_list_next(el)) {
                map = el->data;
                map->bits = g_realloc(map->bits, allocation);
                /* Set newly-allocated bits, if requested */
                memset(map->bits + mgrp->allocated_bytes,
                        map->set_on_extend ? 0xff : 0,
                        allocation - mgrp->allocated_bytes);
            }
            mgrp->allocated_bytes = allocation;
        }

        /* Notify for added bits */
        for (el = g_list_first(mgrp->maps); el != NULL; el = g_list_next(el)) {
            map = el->data;
            for (n = mgrp->nbits; n < bits; n++) {
                if (test_bit(map, n)) {
                    notify_bit(map, n);
                }
            }
        }
    } else if (bits < mgrp->nbits) {
        /* Set removed bits, if requested */
        for (el = g_list_first(mgrp->maps); el != NULL;
                el = g_list_next(el)) {
            map = el->data;
            if (map->set_on_extend) {
                for (n = bits; n < mgrp->nbits; n++) {
                    set_bit(map, n);
                }
            }
        }
    }

    /* Set new length */
    mgrp->nbits = bits;
    g_mutex_unlock(mgrp->lock);
}

void _vmnetfs_bit_group_close(struct bitmap_group *mgrp)
{
    struct bitmap *map;
    GList *el;

    g_mutex_lock(mgrp->lock);
    for (el = g_list_first(mgrp->maps); el != NULL; el = g_list_next(el)) {
        map = el->data;
        _vmnetfs_stream_group_close(map->sgrp);
    }
    g_mutex_unlock(mgrp->lock);
}

/* All bits are initially set to zero.  For info on set_on_extend, see the
   top of this file. */
struct bitmap *_vmnetfs_bit_new(struct bitmap_group *mgrp, bool set_on_extend)
{
    struct bitmap *map;
    uint64_t n;

    map = g_slice_new0(struct bitmap);
    map->mgrp = mgrp;
    map->sgrp = _vmnetfs_stream_group_new(populate_stream, map);
    map->set_on_extend = set_on_extend;

    g_mutex_lock(mgrp->lock);
    map->bits = g_malloc0(mgrp->allocated_bytes);
    if (set_on_extend) {
        /* Ensure allocated but unused bits are set, in case we resize later */
        for (n = mgrp->nbits; n < 8 * mgrp->allocated_bytes; n++) {
            set_bit(map, n);
        }
    }
    mgrp->maps = g_list_prepend(mgrp->maps, map);
    g_mutex_unlock(mgrp->lock);

    return map;
}

void _vmnetfs_bit_free(struct bitmap *map)
{
    g_mutex_lock(map->mgrp->lock);
    map->mgrp->maps = g_list_remove(map->mgrp->maps, map);
    g_mutex_unlock(map->mgrp->lock);
    _vmnetfs_stream_group_free(map->sgrp);
    g_free(map->bits);
    g_slice_free(struct bitmap, map);
}

/* Setting an out-of-range bit silently fails, to simplify resize races */
void _vmnetfs_bit_set(struct bitmap *map, uint64_t bit)
{
	_vmnetfs_bit_set_force(map, bit, false);
}

void _vmnetfs_bit_set_force(struct bitmap *map, uint64_t bit, bool is_force_notify)
{
    bool is_new = false;

    g_mutex_lock(map->mgrp->lock);
    if (bit < map->mgrp->nbits) {
        is_new = !test_bit(map, bit);
        set_bit(map, bit);
    }
    g_mutex_unlock(map->mgrp->lock);
    if (is_force_notify) {
        notify_bit(map, bit);
    }else {
    	if (is_new) {
    		notify_bit(map, bit);
    	}
    }
}

/* Testing an out-of-range bit returns true to simplify resize races */
bool _vmnetfs_bit_test(struct bitmap *map, uint64_t bit)
{
    bool ret = true;

    g_mutex_lock(map->mgrp->lock);
    if (bit < map->mgrp->nbits) {
        ret = test_bit(map, bit);
    }
    g_mutex_unlock(map->mgrp->lock);
    return ret;
}

struct vmnetfs_stream_group *_vmnetfs_bit_get_stream_group(struct bitmap *map)
{
    return map->sgrp;
}
