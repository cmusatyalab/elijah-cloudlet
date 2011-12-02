/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
 *
 * Copyright (C) 2006-2011 Carnegie Mellon University
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
#include <time.h>
#include <errno.h>
#include "defs.h"
#include "fuse_defs.h"

#define MAX_CACHE_MULT 1
#define MAX_CACHE_DIV 10
#define DIRTY_WRITEBACK_DELAY 5 /* seconds */

struct cache_entry {
	/* Protected by image lock */
	unsigned chunk;
	gboolean busy;
	unsigned waiters;
	GCond *available;
	GList *dirty_link;
	GList *reclaimable_link;

	/* Protected by busy flag */
	void *data;		/* NULL if no buffer allocated */
	time_t dirty;		/* 0 if clean */
	gboolean error;
};

struct io_cursor {
	/* Public fields; do not modify */
	unsigned chunk;
	unsigned offset;
	unsigned length;
	unsigned buf_offset;
	gboolean eof;		/* Tried to do I/O past the end of the disk */

	/* Private fields */
	struct pk_state *state;
	off_t start;
	size_t count;
};


static void queue_push_tail_with_link(GQueue *queue, GList **link, void *data)
{
	g_assert(*link == NULL);
	*link = g_list_alloc();
	(*link)->data = data;
	g_queue_push_tail_link(queue, *link);
}

static void queue_delete_with_link(GQueue *queue, GList **link)
{
	if (*link != NULL) {
		g_queue_delete_link(queue, *link);
		*link = NULL;
	}
}

/* Set error flag for an entry.  Busy flag must be held. */
static void entry_set_error(struct pk_state *state, struct cache_entry *ent)
{
	stats_increment(state, chunk_errors, 1);
	state->fuse->leave_dirty = TRUE;
	ent->error = TRUE;
}

/* Clean a dirty entry.  Busy flag and image lock must be held.  Image
   lock will be released and reacquired.  @ent must not have been removed
   from the dirty list. */
static void entry_clean(struct pk_state *state, struct cache_entry *ent)
{
	g_assert(ent->dirty > 0);
	g_assert(ent->dirty_link != NULL);
	g_assert(ent->data != NULL);

	/* Remove the chunk from the dirty list early, so that if we're
	   doing demand reclaim, the cleaner thread knows it doesn't have
	   to deal with this chunk.  Removing this entry can only move the
	   next cleaner wakeup *later*, so we don't bother signalling the
	   cleaner here. */
	queue_delete_with_link(state->fuse->image.dirty, &ent->dirty_link);

	g_mutex_unlock(state->fuse->image.lock);
	if (cache_update(state, ent->chunk, ent->data))
		entry_set_error(state, ent);
	g_mutex_lock(state->fuse->image.lock);

	ent->dirty = 0;
	cache_shm_set_cache_dirty(state, ent->chunk, FALSE);
}

/* Acquire the busy flag for an entry.  Image lock must be held, and may be
   released and reacquired. */
static void _entry_acquire(struct pk_state *state, struct cache_entry *ent)
{
	queue_delete_with_link(state->fuse->image.reclaimable,
				&ent->reclaimable_link);
	ent->waiters++;
	while (ent->busy)
		g_cond_wait(ent->available, state->fuse->image.lock);
	ent->busy = TRUE;
	ent->waiters--;
}

/* Release the busy flag for an entry.  Image lock must be held. */
static void _entry_release(struct pk_state *state, struct cache_entry *ent)
{
	ent->busy = FALSE;
	if (ent->waiters > 0) {
		g_cond_signal(ent->available);
	} else if (ent->data == NULL) {
		/* No data buffer and no waiters; there's no reason to
		   keep this cache entry around anymore. */
		g_assert(ent->reclaimable_link == NULL);
		g_hash_table_remove(state->fuse->image.chunks, &ent->chunk);
		g_cond_free(ent->available);
		g_slice_free(struct cache_entry, ent);
	} else {
		/* We have cached data but no waiters.  Make this entry
		   reclaimable. */
		queue_push_tail_with_link(state->fuse->image.reclaimable,
					&ent->reclaimable_link, ent);
		g_cond_signal(state->fuse->image.reclaimable_cond);
	}
}

/* Get a cache_entry for the specified @chunk, acquire its busy flag,
   allocate a buffer if necessary, populate the buffer with chunk data if
   requested, and return the entry. */
static struct cache_entry *entry_acquire(struct pk_state *state,
			unsigned chunk, gboolean with_data)
{
	struct cache_entry *ent;
	struct cache_entry *reclaim;

	/* Obtain a cache_entry and get its busy flag. */
	g_mutex_lock(state->fuse->image.lock);
	ent = g_hash_table_lookup(state->fuse->image.chunks, &chunk);
	if (ent == NULL) {
		ent = g_slice_new0(struct cache_entry);
		ent->chunk = chunk;
		ent->available = g_cond_new();
		g_hash_table_replace(state->fuse->image.chunks, &ent->chunk,
					ent);
	}
	_entry_acquire(state, ent);

	if (ent->data == NULL) {
		/* This entry has no buffer.  Get one. */
		if (state->fuse->image.allocatable > 0) {
			state->fuse->image.allocatable--;
			ent->data = g_slice_alloc(state->parcel->chunksize);
		} else {
			while (g_queue_is_empty(state->fuse->
						image.reclaimable))
				g_cond_wait(state->fuse->
						image.reclaimable_cond,
						state->fuse->image.lock);
			/* _entry_acquire() will pop, so we just peek */
			reclaim = g_queue_peek_head(
					state->fuse->image.reclaimable);
			pk_log(LOG_FUSE, "Reclaim: %u", reclaim->chunk);
			stats_increment(state, cache_evictions, 1);
			_entry_acquire(state, reclaim);
			if (reclaim->dirty) {
				entry_clean(state, reclaim);
				stats_increment(state, cache_evictions_dirty,
							1);
			}
			g_assert(reclaim->data != NULL);
			ent->data = reclaim->data;
			reclaim->data = NULL;
			cache_shm_set_cached(state, reclaim->chunk, FALSE);
			_entry_release(state, reclaim);
		}
		cache_shm_set_cached(state, ent->chunk, TRUE);
		g_mutex_unlock(state->fuse->image.lock);

		/* Populate it if requested. */
		if (with_data) {
			if (cache_get(state, ent->chunk, ent->data))
				entry_set_error(state, ent);
			stats_increment(state, cache_misses, 1);
		}
	} else {
		g_mutex_unlock(state->fuse->image.lock);
		if (with_data)
			stats_increment(state, cache_hits, 1);
	}
	return ent;
}

/* Mark the entry dirty, if requested, and release its busy flag. */
static void entry_release(struct pk_state *state, struct cache_entry *ent,
			gboolean dirty)
{
	if (dirty)
		cache_shm_set_dirty(state, ent->chunk);
	g_mutex_lock(state->fuse->image.lock);
	if (dirty && !ent->dirty) {
		ent->dirty = time(NULL);
		queue_push_tail_with_link(state->fuse->image.dirty,
					&ent->dirty_link, ent);
		if (g_queue_peek_head(state->fuse->image.dirty) == ent) {
			/* We've changed the queue head, so the cleaner
			   needs to recalculate its wakeup time */
			g_cond_signal(state->fuse->cleaner.cond);
		}
		cache_shm_set_cache_dirty(state, ent->chunk, TRUE);
	}
	_entry_release(state, ent);
	g_mutex_unlock(state->fuse->image.lock);
}

/* Clean all dirty entries that are ripe for writeback.  If @force is TRUE,
   clean all dirty entries.  Image lock must be held. */
static void entry_clean_all(struct pk_state *state, gboolean force)
{
	struct cache_entry *ent;

	while ((ent = g_queue_peek_head(state->fuse->image.dirty))) {
		_entry_acquire(state, ent);
		if (!ent->dirty) {
			/* By the time we acquired the busy flag, the
			   chunk was no longer dirty. */
			_entry_release(state, ent);
			continue;
		}
		if (!force && ent->dirty + DIRTY_WRITEBACK_DELAY >
					time(NULL)) {
			_entry_release(state, ent);
			break;
		}
		entry_clean(state, ent);
		_entry_release(state, ent);
	}
}

/* Thread to write dirty entries back to disk */
static void *entry_cleaner(void *data)
{
	struct pk_state *state = data;
	struct cache_entry *ent;
	GTimeVal timeout;

	g_mutex_lock(state->fuse->image.lock);
	while (!state->fuse->image.stopping) {
		/* Clean what we can */
		entry_clean_all(state, FALSE);

		/* Sleep until we're needed again */
		ent = g_queue_peek_head(state->fuse->image.dirty);
		if (ent != NULL) {
			/* Set wakeup based on the expiration time of the
			   head-of-queue.  Round off for better energy use. */
			g_get_current_time(&timeout);
			timeout.tv_sec += ent->dirty + DIRTY_WRITEBACK_DELAY -
						time(NULL);
			timeout.tv_usec = 0;
			g_cond_timed_wait(state->fuse->cleaner.cond,
						state->fuse->image.lock,
						&timeout);
		} else {
			/* No dirty chunks.  We'll be woken when one
			   arrives. */
			g_cond_wait(state->fuse->cleaner.cond,
						state->fuse->image.lock);
		}
	}
	g_mutex_unlock(state->fuse->image.lock);
	return NULL;
}

static void _image_shutdown(struct pk_state *state)
{
	g_cond_free(state->fuse->cleaner.cond);
	g_cond_free(state->fuse->image.reclaimable_cond);
	g_queue_free(state->fuse->image.reclaimable);
	g_queue_free(state->fuse->image.dirty);
	g_hash_table_destroy(state->fuse->image.chunks);
	g_mutex_free(state->fuse->image.lock);
}

void image_shutdown(struct pk_state *state)
{
	struct cache_entry *ent;

	/* Stop the cleaner thread */
	g_mutex_lock(state->fuse->image.lock);
	state->fuse->image.stopping = TRUE;
	g_cond_broadcast(state->fuse->cleaner.cond);
	g_mutex_unlock(state->fuse->image.lock);
	g_thread_join(state->fuse->cleaner.thread);

	/* Write back dirty chunks */
	g_mutex_lock(state->fuse->image.lock);
	entry_clean_all(state, TRUE);

	/* Free chunk buffers */
	while ((ent = g_queue_peek_head(state->fuse->image.reclaimable))) {
		_entry_acquire(state, ent);
		g_assert(!ent->dirty);
		g_assert(ent->data != NULL);
		g_slice_free1(state->parcel->chunksize, ent->data);
		ent->data = NULL;
		cache_shm_set_cached(state, ent->chunk, FALSE);
		/* Since the entry has no buffer, it will be freed */
		_entry_release(state, ent);
	}
	g_assert(g_hash_table_size(state->fuse->image.chunks) == 0);
	g_mutex_unlock(state->fuse->image.lock);

	/* Free data structures */
	_image_shutdown(state);
}

pk_err_t image_init(struct pk_state *state)
{
	GError *err = NULL;
	unsigned max_mb;

	max_mb = (uint64_t) MAX_CACHE_MULT * sysconf(_SC_PHYS_PAGES) *
				sysconf(_SC_PAGE_SIZE) / (MAX_CACHE_DIV << 20);
	if (state->conf->chunk_cache == 0) {
		pk_log(LOG_WARNING, "Chunk cache size may not be zero");
		return PK_INVALID;
	}
	if (state->conf->chunk_cache > max_mb) {
		pk_log(LOG_WARNING, "Chunk cache may not be larger than "
					G_STRINGIFY(MAX_CACHE_MULT) "/"
					G_STRINGIFY(MAX_CACHE_DIV)
					" of system RAM (%u MB)", max_mb);
		return PK_INVALID;
	}

	state->fuse->image.lock = g_mutex_new();
	state->fuse->image.chunks = g_hash_table_new(g_int_hash, g_int_equal);
	state->fuse->image.dirty = g_queue_new();
	state->fuse->image.reclaimable = g_queue_new();
	state->fuse->image.reclaimable_cond = g_cond_new();
	state->fuse->image.allocatable = state->conf->chunk_cache *
				((1 << 20) / state->parcel->chunksize);
	pk_log(LOG_INFO, "Chunk cache: %u entries",
				state->fuse->image.allocatable);

	state->fuse->cleaner.cond = g_cond_new();
	state->fuse->cleaner.thread = g_thread_create(entry_cleaner, state,
				TRUE, &err);
	if (state->fuse->cleaner.thread == NULL) {
		pk_log(LOG_ERROR, "Couldn't create cleaner thread: %s",
					err->message);
		g_clear_error(&err);
		_image_shutdown(state);
		return PK_CALLFAIL;
	}
	return PK_SUCCESS;
}

/* The cursor is assumed to be allocated on the stack; this just fills
   it in. */
static void io_start(struct pk_state *state, struct io_cursor *cur,
			off_t start, size_t count)
{
	memset(cur, 0, sizeof(*cur));
	cur->state = state;
	cur->start = start;
	cur->count = count;
}

/* Populate the public fields of the cursor with information on the next
   chunk in the I/O, starting from the first.  Returns TRUE if we produced
   a valid chunk, FALSE if done with this I/O. */
static gboolean io_chunk(struct io_cursor *cur)
{
	cur->buf_offset += cur->length;
	if (cur->buf_offset >= cur->count)
		return FALSE;  /* Done */
	cur->chunk = (cur->start + cur->buf_offset) /
				cur->state->parcel->chunksize;
	if (cur->chunk >= cur->state->parcel->chunks) {
		/* End of disk */
		cur->eof = TRUE;
		return FALSE;
	}
	cur->offset = cur->start + cur->buf_offset -
				(cur->chunk * cur->state->parcel->chunksize);
	cur->length = MIN(cur->state->parcel->chunksize - cur->offset,
				cur->count - cur->buf_offset);
	return TRUE;
}

int image_read(struct pk_state *state, char *buf, off_t start, size_t count)
{
	struct io_cursor cur;
	struct cache_entry *ent;

	pk_log(LOG_FUSE, "Read %"PRIu64" at %"PRIu64, (uint64_t) count,
				(uint64_t) start);
	for (io_start(state, &cur, start, count); io_chunk(&cur); ) {
		ent = entry_acquire(state, cur.chunk, TRUE);
		if (ent->error) {
			entry_release(state, ent, FALSE);
			return (int) cur.buf_offset ?: -EIO;
		}
		memcpy(buf + cur.buf_offset, ent->data + cur.offset,
					cur.length);
		entry_release(state, ent, FALSE);
		stats_increment(state, bytes_read, cur.length);
	}
	return cur.buf_offset;
}

int image_write(struct pk_state *state, const char *buf, off_t start,
			size_t count)
{
	struct io_cursor cur;
	struct cache_entry *ent;
	gboolean whole_chunk;

	pk_log(LOG_FUSE, "Write %"PRIu64" at %"PRIu64, (uint64_t) count,
				(uint64_t) start);
	for (io_start(state, &cur, start, count); io_chunk(&cur); ) {
		whole_chunk = cur.length == state->parcel->chunksize;
		ent = entry_acquire(state, cur.chunk, !whole_chunk);
		if (ent->error) {
			entry_release(state, ent, FALSE);
			return (int) cur.buf_offset ?: -EIO;
		}
		memcpy(ent->data + cur.offset, buf + cur.buf_offset,
					cur.length);
		entry_release(state, ent, TRUE);
		stats_increment(state, bytes_written, cur.length);
		if (whole_chunk)
			stats_increment(state, whole_chunk_updates, 1);
	}
	if (cur.eof && !cur.buf_offset)
		return -ENOSPC;
	return cur.buf_offset;
}

void image_sync(struct pk_state *state)
{
	g_mutex_lock(state->fuse->image.lock);
	entry_clean_all(state, TRUE);
	g_mutex_unlock(state->fuse->image.lock);
}
