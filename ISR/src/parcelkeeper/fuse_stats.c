/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
 *
 * Copyright (C) 2006-2010 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include "defs.h"
#include "fuse_defs.h"

typedef gboolean (stat_handler)(void *data, const char *name);

#define RETURN_FORMAT(lock, str, args...) do {	\
	g_mutex_lock(lock);			\
	ret = g_strdup_printf(str, ## args);	\
	g_mutex_unlock(lock);			\
	return ret;				\
} while (0)

static gchar *_statistic(struct pk_state *state, stat_handler *handle,
			void *data)
{
	gchar *ret;

	if (handle(data, "bytes_read"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.bytes_read);
	if (handle(data, "bytes_written"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.bytes_written);
	if (handle(data, "chunk_reads"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.chunk_reads);
	if (handle(data, "chunk_writes"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.chunk_writes);
	if (handle(data, "chunk_errors"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.chunk_errors);
	if (handle(data, "cache_hits"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.cache_hits);
	if (handle(data, "cache_misses"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.cache_misses);
	if (handle(data, "cache_evictions"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.cache_evictions);
	if (handle(data, "cache_evictions_dirty"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.cache_evictions_dirty);
	if (handle(data, "cache_dirty"))
		RETURN_FORMAT(state->fuse->image.lock, "%u\n",
					g_queue_get_length(state->fuse->
					image.dirty));
	if (handle(data, "compression_ratio_pct")) {
		g_mutex_lock(state->stats_lock);
		if (state->stats.chunk_writes)
			ret = g_strdup_printf("%.1f\n", 100.0 *
					state->stats.data_bytes_written /
					(state->stats.chunk_writes *
					state->parcel->chunksize));
		else
			ret = g_strdup("n/a\n");
		g_mutex_unlock(state->stats_lock);
		return ret;
	}
	if (handle(data, "whole_chunk_updates"))
		RETURN_FORMAT(state->stats_lock, "%"PRIu64"\n",
					state->stats.whole_chunk_updates);
	return NULL;
}

#undef RETURN_FORMAT

static gboolean _stat_enumerate(void *data, const char *name)
{
	g_ptr_array_add(data, g_strdup(name));
	return FALSE;
}

gchar **stat_list(struct pk_state *state)
{
	GPtrArray *arr;

	arr = g_ptr_array_new();
	_statistic(state, _stat_enumerate, arr);
	g_ptr_array_add(arr, NULL);
	return (gchar **) g_ptr_array_free(arr, FALSE);
}

static gboolean _stat_compare(void *data, const char *name)
{
	return g_str_equal(data, name);
}

gchar *stat_get(struct pk_state *state, const char *name)
{
	return _statistic(state, _stat_compare, (char *) name);
}

int stat_open(struct pk_state *state, const char *name)
{
	gchar *buf;
	int fh;

	buf = stat_get(state, name);
	if (buf == NULL)
		return -ENOENT;
	g_mutex_lock(state->fuse->stat_buffer_lock);
	/* Find the next available file handle */
	for (fh = 1; g_hash_table_lookup(state->fuse->stat_buffers,
				GINT_TO_POINTER(fh)); fh++);
	g_hash_table_insert(state->fuse->stat_buffers, GINT_TO_POINTER(fh),
				buf);
	g_mutex_unlock(state->fuse->stat_buffer_lock);
	return fh;
}

int stat_read(struct pk_state *state, int fh, char *buf, off_t start,
			size_t count)
{
	gchar *src;
	int srclen;
	int len;

	g_mutex_lock(state->fuse->stat_buffer_lock);
	src = g_hash_table_lookup(state->fuse->stat_buffers,
				GINT_TO_POINTER(fh));
	if (src == NULL) {
		g_mutex_unlock(state->fuse->stat_buffer_lock);
		return -EBADF;
	}
	srclen = strlen(src);
	len = MAX(0, MIN((int) count, srclen - start));
	memcpy(buf, src + start, len);
	g_mutex_unlock(state->fuse->stat_buffer_lock);
	return len;
}

void stat_release(struct pk_state *state, int fh)
{
	g_mutex_lock(state->fuse->stat_buffer_lock);
	g_hash_table_remove(state->fuse->stat_buffers, GINT_TO_POINTER(fh));
	g_mutex_unlock(state->fuse->stat_buffer_lock);
}

void stat_init(struct pk_state *state)
{
	state->fuse->stat_buffers = g_hash_table_new_full(g_direct_hash,
				g_direct_equal, NULL, g_free);
	state->fuse->stat_buffer_lock = g_mutex_new();
}

void stat_shutdown(struct pk_state *state, gboolean normal)
{
	gchar **stats;
	gchar **cur;
	gchar *value;

	if (normal) {
		/* Log statistics */
		for (stats = cur = stat_list(state); *cur != NULL; cur++) {
			value = stat_get(state, *cur);
			g_strchomp(value);
			pk_log(LOG_STATS, "%s: %s", *cur, value);
			g_free(value);
		}
		g_strfreev(stats);
	}
	g_hash_table_destroy(state->fuse->stat_buffers);
	g_mutex_free(state->fuse->stat_buffer_lock);
}
