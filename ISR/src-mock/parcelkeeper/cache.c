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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "defs.h"
#include "config.h"

#define CA_MAGIC 0x51528038
#define CA_VERSION 1
#define CA_INDEX_VERSION 1

/* All u32's in network byte order */
struct ca_header {
	uint32_t magic;
	uint32_t entries;
	uint32_t offset;  /* beginning of data, in 512-byte blocks */
	uint32_t flags;
	uint32_t reserved_1;
	uint8_t version;
};

struct pk_shm {
	gchar *name;
	unsigned char *base;
	unsigned len;
	GMutex *lock;
};

enum shm_chunk_status {
	SHM_PRESENT		= 0x01,
	SHM_DIRTY		= 0x02,
	SHM_ACCESSED_SESSION	= 0x04,
	SHM_DIRTY_SESSION	= 0x08,
	SHM_CACHED		= 0x10,
	SHM_CACHE_DIRTY		= 0x20,
};

static off64_t cache_chunk_to_offset(struct pk_state *state, unsigned chunk)
{
	return (off64_t)state->parcel->chunksize * chunk + state->offset;
}

static pk_err_t set_cache_file_size(struct pk_state *state, int fd)
{
	off64_t len = cache_chunk_to_offset(state, state->parcel->chunks);

#ifdef HAVE_FALLOCATE
	if (!fallocate(fd, 0, 0, len)) {
		return PK_SUCCESS;
	} else {
		int err = errno;

		switch (err) {
		case ENOSYS:
		case EOPNOTSUPP:
			break;
		case ENOSPC:
			pk_log(LOG_INFO, "Not preallocating cache file "
						"due to insufficient space");
			break;
		default:
			pk_log(LOG_WARNING, "Couldn't preallocate cache "
						"file: %s", strerror(err));
			break;
		}
	}
#endif
	if (ftruncate(fd, len)) {
		pk_log(LOG_ERROR, "couldn't extend cache file");
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

static pk_err_t create_cache_file(struct pk_state *state)
{
	struct ca_header hdr = {0};
	int fd;

	fd=open(state->conf->cache_file, O_CREAT|O_EXCL|O_RDWR, 0600);
	if (fd == -1) {
		pk_log(LOG_ERROR, "couldn't create cache file");
		return PK_IOERR;
	}
	/* Place the first chunk 4 KB into the file, for better performance
	   on disks with 4 KB sectors */
	state->offset=4096;
	state->cache_flags=0;
	hdr.magic=htonl(CA_MAGIC);
	hdr.entries=htonl(state->parcel->chunks);
	hdr.offset=htonl(state->offset >> 9);
	hdr.flags=htonl(state->cache_flags);
	hdr.version=CA_VERSION;
	if (set_cache_file_size(state, fd))
		return PK_IOERR;
	if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		pk_log(LOG_ERROR, "Couldn't write cache file header");
		return PK_IOERR;
	}

	pk_log(LOG_INFO, "Created cache file");
	state->cache_fd=fd;
	return PK_SUCCESS;
}

static pk_err_t open_cache_file(struct pk_state *state)
{
	struct ca_header hdr;
	int fd;

	fd=open(state->conf->cache_file, O_RDWR);
	if (fd == -1) {
		pk_log(LOG_ERROR, "couldn't open cache file");
		return PK_IOERR;
	}
	if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		pk_log(LOG_ERROR, "Couldn't read cache file header");
		return PK_IOERR;
	}
	if (ntohl(hdr.magic) != CA_MAGIC) {
		pk_log(LOG_ERROR, "Invalid magic number reading cache file");
		return PK_BADFORMAT;
	}
	if (hdr.version != CA_VERSION) {
		pk_log(LOG_ERROR, "Invalid version reading cache file: "
					"expected %d, found %d", CA_VERSION,
					hdr.version);
		return PK_BADFORMAT;
	}
	if (ntohl(hdr.entries) != state->parcel->chunks) {
		pk_log(LOG_ERROR, "Invalid chunk count reading cache file: "
					"expected %u, found %u",
					state->parcel->chunks,
					ntohl(hdr.entries));
		return PK_BADFORMAT;
	}
	state->cache_flags=ntohl(hdr.flags);
	state->offset=ntohl(hdr.offset) << 9;

	pk_log(LOG_INFO, "Read cache header");
	state->cache_fd=fd;
	return PK_SUCCESS;
}

static pk_err_t cache_set_flags(struct pk_state *state, unsigned flags)
{
	unsigned tmp;

	if (!(state->conf->flags & WANT_LOCK)) {
		/* Catch misuse of this function */
		pk_log(LOG_ERROR, "Refusing to set cache flags when lock "
					"not held");
		return PK_BUSY;
	}
	if (!state->cache_fd) {
		pk_log(LOG_ERROR, "Cache file not open; can't set flags");
		return PK_IOERR;
	}

	tmp=htonl(flags);
	if (pwrite(state->cache_fd, &tmp, sizeof(tmp),
				offsetof(struct ca_header, flags))
				!= sizeof(tmp)) {
		pk_log(LOG_ERROR, "Couldn't write new flags to cache file");
		return PK_IOERR;
	}
	if (fdatasync(state->cache_fd)) {
		pk_log(LOG_ERROR, "Couldn't sync cache file");
		return PK_IOERR;
	}
	state->cache_flags=flags;
	return PK_SUCCESS;
}

pk_err_t cache_set_flag(struct pk_state *state, unsigned flag)
{
	if ((flag & CA_F_DAMAGED) == CA_F_DAMAGED)
		pk_log(LOG_WARNING, "Setting damaged flag on local cache");
	return cache_set_flags(state, state->cache_flags | flag);
}

pk_err_t cache_clear_flag(struct pk_state *state, unsigned flag)
{
	return cache_set_flags(state, state->cache_flags & ~flag);
}

int cache_test_flag(struct pk_state *state, unsigned flag)
{
	return ((state->cache_flags & flag) == flag);
}

static pk_err_t create_cache_index(struct pk_state *state)
{
	gboolean retry;

again:
	if (!begin(state->db))
		return PK_IOERR;
	if (!query(NULL, state->db, "CREATE TABLE cache.chunks ("
				"chunk INTEGER PRIMARY KEY NOT NULL, "
				"length INTEGER NOT NULL)", NULL)) {
		sql_log_err(state->db, "Couldn't create cache index");
		goto bad;
	}
	if (!query(NULL, state->db, "PRAGMA cache.user_version = "
				G_STRINGIFY(CA_INDEX_VERSION), NULL)) {
		sql_log_err(state->db, "Couldn't set cache index version");
		goto bad;
	}
	if (!commit(state->db))
		goto bad;
	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return PK_IOERR;
}

static pk_err_t verify_cache_index(struct pk_state *state)
{
	struct query *qry;
	int found;
	gboolean retry;

again:
	if (!begin(state->db))
		return PK_IOERR;
	query(&qry, state->db, "PRAGMA cache.user_version", NULL);
	if (!query_has_row(state->db)) {
		sql_log_err(state->db, "Couldn't query cache index version");
		goto bad;
	}
	query_row(qry, "d", &found);
	query_free(qry);
	rollback(state->db);
	if (found != CA_INDEX_VERSION) {
		pk_log(LOG_ERROR, "Invalid version reading cache index: "
					"expected %d, found %d",
					CA_INDEX_VERSION, found);
		return PK_BADFORMAT;
	}
	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return PK_SQLERR;
}

/* Must be thread-safe */
static void shm_update(struct pk_state *state, unsigned chunk, unsigned set,
				unsigned clear)
{
	if (state->shm == NULL)
		return;
	if (chunk > state->parcel->chunks) {
		pk_log(LOG_ERROR, "Invalid chunk %u", chunk);
		return;
	}
	g_mutex_lock(state->shm->lock);
	state->shm->base[chunk] |= set;
	state->shm->base[chunk] &= ~clear;
	g_mutex_unlock(state->shm->lock);
}

/* cache_update() sets the chunk dirty when it is called, but that may not be
   for several seconds after the chunk is actually dirtied due to the
   fuse_image.c writeback cache.  Provide a mechanism for the chunk cache to
   notify us that it has just dirtied a chunk so that we can update the
   shm segment immediately. */
void cache_shm_set_dirty(struct pk_state *state, unsigned chunk)
{
	shm_update(state, chunk, SHM_PRESENT | SHM_ACCESSED_SESSION |
				SHM_DIRTY | SHM_DIRTY_SESSION, 0);
}

void cache_shm_set_cached(struct pk_state *state, unsigned chunk,
				gboolean cached)
{
	shm_update(state, chunk, cached ? SHM_CACHED : 0,
				cached ? 0 : SHM_CACHED);
}

void cache_shm_set_cache_dirty(struct pk_state *state, unsigned chunk,
				gboolean dirty)
{
	shm_update(state, chunk, dirty ? SHM_CACHE_DIRTY : 0,
				dirty ? 0 : SHM_CACHE_DIRTY);
}

static pk_err_t shm_init(struct pk_state *state)
{
	int fd;
	struct query *qry;
	unsigned chunk;
	pk_err_t ret;
	gboolean retry;

	state->shm = g_slice_new0(struct pk_shm);
	state->shm->lock = g_mutex_new();
	state->shm->len = state->parcel->chunks;
	state->shm->name = g_strdup_printf("/openisr-chunkmap-%s",
				state->parcel->uuid);
	/* If there's a segment by that name, it's leftover and should be
	   killed.  (Or else we have a UUID collision, which will prevent
	   Nexus registration from succeeding in any case.)  This is racy
	   with regard to someone else deleting and recreating the segment,
	   but we do this under the PK lock so it shouldn't be a problem. */
	shm_unlink(state->shm->name);
	fd=shm_open(state->shm->name, O_RDWR|O_CREAT|O_EXCL, 0400);
	if (fd == -1) {
		pk_log(LOG_ERROR, "Couldn't create shared memory segment: %s",
					strerror(errno));
		ret=PK_IOERR;
		goto bad_open;
	}
	if (ftruncate(fd, state->shm->len)) {
		pk_log(LOG_ERROR, "Couldn't set shared memory segment to "
					"%u bytes", state->shm->len);
		close(fd);
		ret=PK_IOERR;
		goto bad_truncate;
	}
	state->shm->base=mmap(NULL, state->shm->len, PROT_READ|PROT_WRITE,
				MAP_SHARED, fd, 0);
	close(fd);
	if (state->shm->base == MAP_FAILED) {
		pk_log(LOG_ERROR, "Couldn't map shared memory segment");
		ret=PK_CALLFAIL;
		goto bad_truncate;
	}

again:
	if (!begin(state->db)) {
		ret=PK_IOERR;
		goto bad_populate;
	}
	for (query(&qry, state->db, "SELECT chunk FROM cache.chunks", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "d", &chunk);
		shm_update(state, chunk, SHM_PRESENT, 0);
	}
	query_free(qry);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Couldn't query cache index");
		ret=PK_SQLERR;
		goto bad_sql;
	}

	for (query(&qry, state->db, "SELECT main.keys.chunk "
				"FROM main.keys JOIN prev.keys "
				"ON main.keys.chunk == prev.keys.chunk "
				"WHERE main.keys.tag != prev.keys.tag", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "d", &chunk);
		shm_update(state, chunk, SHM_DIRTY, 0);
	}
	query_free(qry);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Couldn't find modified chunks");
		ret=PK_SQLERR;
		goto bad_sql;
	}
	rollback(state->db);
	return PK_SUCCESS;

bad_sql:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
bad_populate:
	munmap(state->shm->base, state->shm->len);
bad_truncate:
	shm_unlink(state->shm->name);
bad_open:
	g_free(state->shm->name);
	g_mutex_free(state->shm->lock);
	g_slice_free(struct pk_shm, state->shm);
	state->shm = NULL;
	return ret;
}

void cache_shutdown(struct pk_state *state)
{
	if (state->shm) {
		g_mutex_free(state->shm->lock);
		munmap(state->shm->base, state->shm->len);
		shm_unlink(state->shm->name);
		g_free(state->shm->name);
		g_slice_free(struct pk_shm, state->shm);
	}
	if (state->cache_fd)
		close(state->cache_fd);
	sql_conn_close(state->db);
}

static pk_err_t open_cachedir(struct pk_state *state)
{
	pk_err_t ret;
	gboolean have_image;
	gboolean have_index;

	if (!sql_conn_open(state->conf->keyring, &state->db))
		return PK_IOERR;

	have_image=g_file_test(state->conf->cache_file, G_FILE_TEST_IS_REGULAR);
	have_index=g_file_test(state->conf->cache_index, G_FILE_TEST_IS_REGULAR);
	if (have_image && have_index) {
		if (!attach(state->db, "cache", state->conf->cache_index))
			return PK_IOERR;
		ret=open_cache_file(state);
		if (ret)
			return ret;
		ret=verify_cache_index(state);
		if (ret)
			return ret;
	} else if ((state->conf->flags & WANT_LOCK) &&
				((have_image && !have_index) ||
				(!have_image && have_index))) {
		/* We don't complain about this unless we have the PK lock,
		   since otherwise we're open to race conditions with another
		   process that does.  If we don't have the PK lock, we just
		   treat this case as though neither image nor index exists. */
		pk_log(LOG_ERROR, "Cache and index in inconsistent state");
		return PK_IOERR;
	} else {
		if (state->conf->flags & WANT_LOCK) {
			if (!attach(state->db, "cache",
						state->conf->cache_index))
				return PK_IOERR;
			ret=create_cache_file(state);
			if (ret)
				return ret;
		} else {
			/* If we WANT_CACHE but don't WANT_LOCK, we need to
			   make sure not to create the image and index files
			   to avoid race conditions.  (Right now this only
			   affects examine mode.)  Create a fake cache index
			   to simplify queries elsewhere. */
			if (!attach(state->db, "cache", ":memory:"))
				return PK_IOERR;
		}
		ret=create_cache_index(state);
		if (ret)
			return ret;
	}
	return PK_SUCCESS;
}

pk_err_t cache_init(struct pk_state *state)
{
	pk_err_t ret;

	if (state->conf->flags & WANT_CACHE) {
		ret=open_cachedir(state);
		if (ret)
			goto bad;
	} else {
		if (!sql_conn_open(":memory:", &state->db)) {
			ret=PK_IOERR;
			goto bad;
		}
	}

	if (state->conf->flags & WANT_PREV) {
		if (!attach(state->db, "prev", state->conf->prev_keyring)) {
			ret=PK_IOERR;
			goto bad;
		}
	}

	if (state->conf->flags & WANT_SHM)
		if (shm_init(state))
			pk_log(LOG_ERROR, "Couldn't set up shared memory "
						"segment; continuing");

	interrupter_add(state->db);
	return PK_SUCCESS;

bad:
	cache_shutdown(state);
	return ret;
}

pk_err_t _cache_read_chunk(struct pk_state *state, unsigned chunk,
			void *buf, unsigned chunklen, const void *tag)
{
	char calctag[state->parcel->hashlen];

	if (pread(state->cache_fd, buf, chunklen, cache_chunk_to_offset(state,
				chunk)) != (int)chunklen) {
		pk_log(LOG_ERROR, "Chunk %u: couldn't read from local cache",
					chunk);
		return PK_IOERR;
	}
	if (tag != NULL) {
		if (!iu_chunk_crypto_digest(state->parcel->crypto, calctag,
					buf, chunklen))
			return PK_CALLFAIL;
		if (memcmp(tag, calctag, state->parcel->hashlen)) {
			pk_log(LOG_WARNING, "Chunk %u: tag check failure",
						chunk);
			log_tag_mismatch(tag, calctag,
						state->parcel->hashlen);
			return PK_TAGFAIL;
		}
	}
	return PK_SUCCESS;
}

/* Must be in state->db transaction */
static pk_err_t _cache_write_chunk(struct pk_state *state, unsigned chunk,
			const void *buf, unsigned len)
{
	char data[state->parcel->chunksize];
	ssize_t count;

	if (len > state->parcel->chunksize)
		return PK_INVALID;
	/* Write out the entire slot, not just the utilized bytes.  This
	   allows the kernel to coalesce I/O to adjacent chunks.  On
	   systems too old for fallocate(), it may also convince the
	   filesystem to allocate contiguous sectors for the chunk. */
	memcpy(data, buf, len);
	memset(data + len, 0, state->parcel->chunksize - len);
	count = pwrite(state->cache_fd, data, state->parcel->chunksize,
				cache_chunk_to_offset(state, chunk));
	if (count != (int) state->parcel->chunksize) {
		pk_log(LOG_ERROR, "Couldn't write chunk %u to backing store",
					chunk);
		return PK_IOERR;
	}
	/* Update cache index */
	if (!query(NULL, state->db, "INSERT OR REPLACE INTO cache.chunks "
				"(chunk, length) VALUES(?, ?)", "dd",
				chunk, (int)len)) {
		sql_log_err(state->db, "Couldn't update cache index for "
					"chunk %u", chunk);
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

pk_err_t cache_get(struct pk_state *state, unsigned chunk, void *buf)
{
	struct query *qry;
	void *rowtag;
	void *rowkey;
	char encrypted[state->parcel->chunksize];
	char tag[state->parcel->hashlen];
	char key[state->parcel->hashlen];
	unsigned compress;
	unsigned len;
	unsigned taglen;
	unsigned keylen;
	gchar *ftag;
	pk_err_t ret;
	gboolean retry;

	pk_log(LOG_CHUNK, "Get: %u", chunk);
again:
	if (!begin(state->db))
		return PK_IOERR;
	query(&qry, state->db, "SELECT tag, key, compression FROM keys "
				"WHERE chunk == ?", "d", chunk);
	if (!query_has_row(state->db)) {
		sql_log_err(state->db, "Couldn't query keyring");
		ret=PK_IOERR;
		goto bad;
	}
	query_row(qry, "bbd", &rowtag, &taglen, &rowkey, &keylen, &compress);
	if (taglen != state->parcel->hashlen ||
				keylen != state->parcel->hashlen) {
		query_free(qry);
		pk_log(LOG_ERROR, "Invalid hash length for chunk %u: "
					"expected %d, tag %d, key %d",
					chunk, state->parcel->hashlen, taglen,
					keylen);
		ret=PK_INVALID;
		goto bad;
	}
	if (!iu_chunk_compress_is_enabled(state->parcel->required_compress,
				compress)) {
		query_free(qry);
		pk_log(LOG_ERROR, "Invalid or unsupported compression type "
					"for chunk %u: %u", chunk, compress);
		ret=PK_INVALID;
		goto bad;
	}
	memcpy(tag, rowtag, state->parcel->hashlen);
	memcpy(key, rowkey, state->parcel->hashlen);
	query_free(qry);

	if (!query(&qry, state->db, "SELECT length FROM cache.chunks "
				"WHERE chunk == ?", "d", chunk)) {
		sql_log_err(state->db, "Couldn't query cache index");
		ret=PK_IOERR;
		goto bad;
	}
	if (query_has_row(state->db)) {
		/* Chunk is in local cache */
		query_row(qry, "d", &len);
		query_free(qry);
	} else {
		len = 0;
	}

	/* Dispense with the database */
	if (!commit(state->db)) {
		ret=PK_IOERR;
		goto bad;
	}

	if (len) {
		/* Read the chunk from the local cache.  Don't check the
		   tag, since decrypt will check the key */
		ret = _cache_read_chunk(state, chunk, encrypted, len, NULL);
		if (ret)
			return ret;
	} else if (hoard_get_chunk(state, tag, encrypted, &len)) {
		/* Chunk is not in hoard cache; fetch from network */
		ftag = format_tag(tag, state->parcel->hashlen);
		pk_log(LOG_CHUNK, "Tag %s not in hoard cache", ftag);
		g_free(ftag);
		ret = transport_fetch_chunk(state->cpool, encrypted, chunk,
					tag, &len);
		if (ret)
			return ret;
	}

	if (len > state->parcel->chunksize) {
		pk_log(LOG_ERROR, "Invalid chunk length for chunk %u: %u",
					chunk, len);
		return PK_INVALID;
	}


	if (!iu_chunk_decode(state->parcel->crypto, compress, chunk,
				encrypted, len, key, buf,
				state->parcel->chunksize))
		return PK_IOERR;

	stats_increment(state, chunk_reads, 1);
	shm_update(state, chunk, SHM_ACCESSED_SESSION, 0);
	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return ret;
}

pk_err_t cache_update(struct pk_state *state, unsigned chunk, const void *buf)
{
	gboolean retry;
	char encrypted[state->parcel->chunksize];
	char tag[state->parcel->hashlen];
	char key[state->parcel->hashlen];
	unsigned compress;
	unsigned len;

	pk_log(LOG_CHUNK, "Update: %u", chunk);

	compress = state->conf->compress;
	if (!iu_chunk_encode(state->parcel->crypto, buf,
				state->parcel->chunksize, encrypted, &len,
				tag, key, &compress))
		return PK_IOERR;

again:
	if (!begin(state->db))
		return PK_IOERR;
	if (_cache_write_chunk(state, chunk, encrypted, len))
		goto bad;
	if (!query(NULL, state->db, "INSERT OR REPLACE INTO cache.chunks "
				"(chunk, length) VALUES(?, ?)", "dd",
				chunk, len)) {
		sql_log_err(state->db, "Couldn't update cache index");
		goto bad;
	}
	if (!query(NULL, state->db, "UPDATE keys SET tag = ?, key = ?, "
				"compression = ? WHERE chunk == ?", "bbdd",
				tag, state->parcel->hashlen, key,
				state->parcel->hashlen, compress, chunk)) {
		sql_log_err(state->db, "Couldn't update keyring");
		goto bad;
	}
	if (!commit(state->db))
		goto bad;
	stats_increment(state, chunk_writes, 1);
	stats_increment(state, data_bytes_written, len);
	shm_update(state, chunk, SHM_PRESENT | SHM_ACCESSED_SESSION |
				SHM_DIRTY | SHM_DIRTY_SESSION, 0);
	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return PK_IOERR;
}

pk_err_t cache_count_chunks(struct pk_state *state, unsigned *valid,
			unsigned *dirty)
{
	struct query *qry;
	gboolean retry;

again:
	if (!begin(state->db))
		return PK_IOERR;
	if (valid != NULL) {
		query(&qry, state->db, "SELECT count(*) from cache.chunks",
					NULL);
		if (!query_has_row(state->db)) {
			sql_log_err(state->db, "Couldn't query cache index");
			goto bad;
		}
		query_row(qry, "d", valid);
		query_free(qry);
	}
	if (dirty != NULL) {
		query(&qry, state->db, "SELECT count(*) "
					"FROM main.keys JOIN prev.keys ON "
					"main.keys.chunk == prev.keys.chunk "
					"WHERE main.keys.tag != prev.keys.tag",
					NULL);
		if (!query_has_row(state->db)) {
			sql_log_err(state->db, "Couldn't compare keyrings");
			goto bad;
		}
		query_row(qry, "d", dirty);
		query_free(qry);
	}
	/* We didn't make any changes; we just need to release the locks */
	rollback(state->db);
	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return PK_IOERR;
}
