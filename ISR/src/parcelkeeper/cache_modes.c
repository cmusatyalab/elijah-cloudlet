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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "defs.h"

static pk_err_t make_upload_dirs(struct pk_state *state)
{
	gchar *path;
	unsigned dir;
	unsigned numdirs;

	if (!g_file_test(state->conf->dest_dir, G_FILE_TEST_IS_DIR) &&
				mkdir(state->conf->dest_dir, 0700)) {
		pk_log(LOG_ERROR, "Unable to make directory %s",
					state->conf->dest_dir);
		return PK_IOERR;
	}
	numdirs = (state->parcel->chunks + state->parcel->chunks_per_dir - 1) /
				state->parcel->chunks_per_dir;
	for (dir=0; dir < numdirs; dir++) {
		path = g_strdup_printf("%s/%.4d", state->conf->dest_dir, dir);
		if (!g_file_test(path, G_FILE_TEST_IS_DIR) &&
					mkdir(path, 0700)) {
			pk_log(LOG_ERROR, "Unable to make directory %s", path);
			free(path);
			return PK_IOERR;
		}
		g_free(path);
	}
	return PK_SUCCESS;
}

int copy_for_upload(struct pk_state *state)
{
	struct query *qry;
	void *buf;
	unsigned chunk;
	void *tag;
	unsigned taglen;
	unsigned length;
	gchar *path;
	int fd;
	unsigned modified_chunks;
	off64_t modified_bytes;
	int64_t total_modified_bytes;
	int ret=1;
	pk_err_t err;
	gboolean retry;

	if (cache_test_flag(state, CA_F_DAMAGED)) {
		pk_log(LOG_WARNING, "Local cache marked as damaged; "
					"upload disallowed");
		return 1;
	}
	if (cache_test_flag(state, CA_F_DIRTY)) {
		pk_log(LOG_WARNING, "Local cache marked as dirty");
		pk_log(LOG_WARNING, "Will not upload until the cache has "
					"been validated");
		return 1;
	}

	pk_log(LOG_INFO, "Copying chunks to upload directory %s",
				state->conf->dest_dir);
	if (make_upload_dirs(state))
		return 1;
	printf("Updating hoard cache...\n");
	if (hoard_sync_refs(state, TRUE))
		return 1;
	printf("Vacuuming keyring...\n");
	if (!vacuum(state->db))
		return 1;
	buf=g_malloc(state->parcel->chunksize);

	printf("Collecting modified disk state...\n");
again:
	modified_chunks=0;
	modified_bytes=0;
	if (!begin(state->db)) {
		g_free(buf);
		return 1;
	}
	if (!query(NULL, state->db, "CREATE TEMP TABLE to_upload AS "
				"SELECT main.keys.chunk AS chunk, "
				"main.keys.tag AS tag, "
				"cache.chunks.length AS length FROM "
				"main.keys JOIN prev.keys ON "
				"main.keys.chunk == prev.keys.chunk "
				"LEFT JOIN cache.chunks ON "
				"main.keys.chunk == cache.chunks.chunk WHERE "
				"main.keys.tag != prev.keys.tag", NULL)) {
		sql_log_err(state->db, "Couldn't enumerate modified chunks");
		goto bad;
	}
	query(&qry, state->db, "SELECT sum(length) FROM temp.to_upload", NULL);
	if (!query_has_row(state->db)) {
		sql_log_err(state->db, "Couldn't find size of modified chunks");
		goto bad;
	}
	query_row(qry, "D", &total_modified_bytes);
	query_free(qry);
	for (query(&qry, state->db, "SELECT chunk, tag, length FROM "
				"temp.to_upload", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "dbd", &chunk, &tag, &taglen, &length);
		print_progress_mb(modified_bytes, total_modified_bytes);
		if (chunk > state->parcel->chunks) {
			pk_log(LOG_WARNING, "Chunk %u: greater than parcel "
						"size %u", chunk,
						state->parcel->chunks);
			goto damaged;
		}
		if (taglen != state->parcel->hashlen) {
			pk_log(LOG_WARNING, "Chunk %u: expected tag length "
						"%u, found %u", chunk,
						state->parcel->hashlen, taglen);
			goto damaged;
		}
		if (length == 0) {
			/* No cache index record */
			pk_log(LOG_WARNING, "Chunk %u: modified but not "
						"present", chunk);
			goto damaged;
		}
		if (length > state->parcel->chunksize) {
			pk_log(LOG_WARNING, "Chunk %u: absurd length %u",
						chunk, length);
			goto damaged;
		}
		err = _cache_read_chunk(state, chunk, buf, length, tag);
		if (err == PK_TAGFAIL)
			goto damaged;
		else if (err)
			goto out;
		path=form_chunk_path(state->parcel, state->conf->dest_dir,
					chunk);
		fd=open(path, O_WRONLY|O_CREAT|O_TRUNC, 0600);
		if (fd == -1) {
			pk_log(LOG_ERROR, "Couldn't open chunk file %s", path);
			g_free(path);
			goto out;
		}
		if (write(fd, buf, length) != (int)length) {
			pk_log(LOG_ERROR, "Couldn't write chunk file %s",
						path);
			g_free(path);
			goto out;
		}
		if (close(fd) && errno != EINTR) {
			pk_log(LOG_ERROR, "Couldn't write chunk file %s",
						path);
			g_free(path);
			goto out;
		}
		g_free(path);
		hoard_put_chunk(state, tag, buf, length);
		modified_chunks++;
		modified_bytes += length;
	}
	if (!query_ok(state->db))
		sql_log_err(state->db, "Database query failed");
	else
		ret=0;
out:
	query_free(qry);
bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	g_free(buf);
	if (ret == 0)
		pk_log(LOG_STATS, "Copied %u modified chunks, %llu bytes",
					modified_chunks,
					(unsigned long long) modified_bytes);
	return ret;

damaged:
	cache_set_flag(state, CA_F_DAMAGED);
	goto out;
}

static pk_err_t validate_sqlite(struct pk_state *state, gboolean *ok)
{
	gboolean retry;

	if (validate_db(state->db))
		return PK_SUCCESS;

	/* SQLite database is corrupt.  There have been multiple occasions
	   in which this has been fixable by dropping and recreating
	   indexes, but at least one occasion in which this resulted in an
	   inconsistent database that was prone to corruption later.
	   Thus, only do this if splice is requested, and mark the cache
	   damaged regardless. */
	*ok = FALSE;
	if (!(state->conf->flags & WANT_SPLICE))
		return PK_BADFORMAT;

	pk_log(LOG_WARNING, "Database check failed; trying to "
				"recreate indexes...");
again:
	if (!begin(state->db)) {
		pk_log(LOG_WARNING, "...failed");
		return PK_BADFORMAT;
	}
	if (!query(NULL, state->db, "DROP INDEX keys_tags", NULL)) {
		sql_log_err(state->db, "Couldn't drop keys_tags index");
		goto bad;
	}
	if (!query(NULL, state->db, "CREATE INDEX keys_tags on keys (tag)",
				NULL)) {
		sql_log_err(state->db, "Couldn't create keys_tags index");
		goto bad;
	}
	if (!commit(state->db))
		goto bad;

	pk_log(LOG_WARNING, "Recreated indexes.  Rechecking database...");
	if (!validate_db(state->db))
		return PK_BADFORMAT;

	return PK_SUCCESS;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	return PK_BADFORMAT;
}

static pk_err_t validate_keyring(struct pk_state *state, gboolean *ok)
{
	struct query *qry;
	unsigned expected_chunk;
	unsigned chunk;
	unsigned taglen;
	unsigned keylen;
	unsigned compress;
	pk_err_t ret=PK_SUCCESS;

again:
	expected_chunk=0;
	if (!begin(state->db))
		return PK_IOERR;
	for (query(&qry, state->db, "SELECT chunk, tag, key, compression "
				"FROM keys ORDER BY chunk ASC", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "dnnd", &chunk, &taglen, &keylen, &compress);
		if (chunk >= state->parcel->chunks) {
			pk_log(LOG_WARNING, "Found keyring entry %u greater "
						"than parcel size %u", chunk,
						state->parcel->chunks);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}
		if (chunk < expected_chunk) {
			pk_log(LOG_WARNING, "Found unexpected keyring entry "
						"for chunk %u", chunk);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}
		while (expected_chunk < chunk) {
			pk_log(LOG_WARNING, "Missing keyring entry for chunk "
						"%u", expected_chunk);
			ret=PK_INVALID;
			*ok=FALSE;
			expected_chunk++;
		}
		expected_chunk++;
		if (taglen != state->parcel->hashlen) {
			pk_log(LOG_WARNING, "Chunk %u: expected tag length "
						"%u, found %u", chunk,
						state->parcel->hashlen, taglen);
			ret=PK_INVALID;
			*ok=FALSE;
		}
		if (keylen != state->parcel->hashlen) {
			pk_log(LOG_WARNING, "Chunk %u: expected key length "
						"%u, found %u", chunk,
						state->parcel->hashlen, keylen);
			ret=PK_INVALID;
			*ok=FALSE;
		}
		if (!iu_chunk_compress_is_enabled(
					state->parcel->required_compress,
					compress)) {
			pk_log(LOG_WARNING, "Chunk %u: invalid or unsupported "
						"compression type %u", chunk,
						compress);
			ret=PK_INVALID;
			*ok=FALSE;
		}
	}
	query_free(qry);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Keyring query failed");
		if (query_busy(state->db)) {
			rollback(state->db);
			query_backoff(state->db);
			goto again;
		}
		ret=PK_IOERR;
	}
	rollback(state->db);
	return ret;
}

/* Must be within transaction */
static pk_err_t revert_chunk(struct pk_state *state, int chunk)
{
	pk_log(LOG_WARNING, "Reverting chunk %d", chunk);
	if (!query(NULL, state->db, "INSERT OR REPLACE INTO main.keys "
				"(chunk, tag, key, compression) "
				"SELECT chunk, tag, key, compression FROM "
				"prev.keys WHERE chunk == ?", "d", chunk)) {
		sql_log_err(state->db, "Couldn't revert keyring entry for "
					"chunk %d", chunk);
		return PK_IOERR;
	}
	if (!query(NULL, state->db, "DELETE FROM cache.chunks WHERE chunk == ?",
				"d", chunk)) {
		sql_log_err(state->db, "Couldn't delete cache entry for "
					"chunk %d", chunk);
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

static pk_err_t validate_cachefile(struct pk_state *state, gboolean *ok)
{
	struct query *qry;
	void *buf;
	void *tag;
	unsigned chunk;
	unsigned taglen;
	unsigned chunklen;
	int64_t processed_bytes;
	int64_t valid_bytes;
	pk_err_t ret;
	gboolean retry;

	buf=g_malloc(state->parcel->chunksize);

again:
	processed_bytes=0;
	ret=PK_SUCCESS;
	if (!begin(state->db))
		return PK_IOERR;
	query(&qry, state->db, "SELECT sum(length) FROM cache.chunks", NULL);
	if (!query_has_row(state->db)) {
		sql_log_err(state->db, "Couldn't get total size of valid "
					"chunks");
		ret=PK_IOERR;
		goto bad;
	}
	query_row(qry, "D", &valid_bytes);
	query_free(qry);

	for (query(&qry, state->db, "SELECT main.keys.chunk FROM "
				"main.keys JOIN prev.keys ON "
				"main.keys.chunk == prev.keys.chunk "
				"LEFT JOIN cache.chunks ON "
				"main.keys.chunk == cache.chunks.chunk "
				"WHERE main.keys.tag != prev.keys.tag AND "
				"cache.chunks.chunk ISNULL", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "d", &chunk);
		pk_log(LOG_WARNING, "Chunk %u: modified but not present",
					chunk);
		if (state->conf->flags & WANT_SPLICE) {
			ret=revert_chunk(state, chunk);
			if (ret) {
				query_free(qry);
				goto bad;
			}
		}
		ret=PK_INVALID;
		*ok=FALSE;
	}
	query_free(qry);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Error checking modified chunks");
		ret=PK_IOERR;
		goto bad;
	}

	for (query(&qry, state->db, "SELECT cache.chunks.chunk, "
				"cache.chunks.length, keys.tag FROM "
				"cache.chunks LEFT JOIN keys ON "
				"cache.chunks.chunk == keys.chunk", NULL);
				query_has_row(state->db); query_next(qry)) {
		query_row(qry, "ddb", &chunk, &chunklen, &tag, &taglen);
		processed_bytes += chunklen;
		print_progress_mb(processed_bytes, valid_bytes);

		if (chunk > state->parcel->chunks) {
			pk_log(LOG_WARNING, "Found chunk %u greater than "
						"parcel size %u", chunk,
						state->parcel->chunks);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}
		if (chunklen > state->parcel->chunksize || chunklen == 0) {
			pk_log(LOG_WARNING, "Chunk %u: absurd size %u",
						chunk, chunklen);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}
		if (tag == NULL) {
			pk_log(LOG_WARNING, "Found valid chunk %u with no "
						"keyring entry", chunk);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}
		if (taglen != state->parcel->hashlen) {
			pk_log(LOG_WARNING, "Chunk %u: expected tag length "
						"%u, found %u", chunk,
						state->parcel->hashlen, taglen);
			ret=PK_INVALID;
			*ok=FALSE;
			continue;
		}

		if (state->conf->flags & WANT_FULL_CHECK) {
			ret = _cache_read_chunk(state, chunk, buf, chunklen,
						tag);
			if (ret == PK_TAGFAIL) {
				if (state->conf->flags & WANT_SPLICE) {
					ret=revert_chunk(state, chunk);
					if (ret) {
						query_free(qry);
						goto bad;
					}
				}
				*ok=FALSE;
			}
		}
	}
	query_free(qry);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Error querying cache index");
		ret=PK_IOERR;
		goto bad;
	}
	if (!commit(state->db)) {
		ret=PK_IOERR;
		goto bad;
	}
	g_free(buf);
	return ret;

bad:
	retry = query_busy(state->db);
	rollback(state->db);
	if (retry) {
		query_backoff(state->db);
		goto again;
	}
	g_free(buf);
	return ret;
}

int validate_cache(struct pk_state *state)
{
	int ret=0;
	pk_err_t err;
	gboolean ok=TRUE;

	if (state->conf->flags & WANT_CHECK) {
		/* Don't actually do any validation; just see where we are */
		if (cache_test_flag(state, CA_F_DIRTY))
			ret |= 2;
		if (cache_test_flag(state, CA_F_DAMAGED))
			ret |= 4;
		return ret;
	}

	pk_log(LOG_INFO, "Validating databases");
	printf("Validating databases...\n");
	err=validate_sqlite(state, &ok);
	if (err)
		goto out;

	pk_log(LOG_INFO, "Validating keyring");
	printf("Validating keyring...\n");
	err=validate_keyring(state, &ok);
	if (err)
		goto out;

	pk_log(LOG_INFO, "Checking cache consistency");
	printf("Checking local cache for internal consistency...\n");
	err=validate_cachefile(state, &ok);

out:
	/* !ok means we should set the damaged flag.
	   err != 0 means some test encountered a fatal error (== don't
	   call further test functions), which may or may not be because
	   the cache is damaged. */
	if (ok && err == PK_SUCCESS) {
		if (cache_test_flag(state, CA_F_DIRTY)) {
			if (state->conf->flags & WANT_FULL_CHECK) {
				cache_clear_flag(state, CA_F_DIRTY);
			} else {
				pk_log(LOG_INFO, "Not clearing dirty flag: "
						"full check not requested");
				printf("Not clearing dirty flag: full check "
						"not requested\n");
			}
		}
		return 0;
	} else {
		if (!ok) {
			if (cache_set_flag(state, CA_F_DAMAGED) == PK_SUCCESS)
				cache_clear_flag(state, CA_F_DIRTY);
		}
		return 1;
	}
}

int examine_cache(struct pk_state *state)
{
	unsigned validchunks;
	unsigned dirtychunks;
	unsigned max_mb;
	unsigned valid_mb;
	unsigned dirty_mb;
	unsigned valid_pct;
	unsigned dirty_pct;

	if (cache_count_chunks(state, &validchunks, &dirtychunks))
		return 1;
	max_mb=(((off64_t)state->parcel->chunks) *
				state->parcel->chunksize) >> 20;
	valid_mb=(((off64_t)validchunks) * state->parcel->chunksize) >> 20;
	dirty_mb=(((off64_t)dirtychunks) * state->parcel->chunksize) >> 20;
	valid_pct=(100 * validchunks) / state->parcel->chunks;
	if (validchunks)
		dirty_pct=(100 * dirtychunks) / validchunks;
	else
		dirty_pct=0;
	printf("Local cache : %u%% populated (%u/%u MB), %u%% modified "
				"(%u/%u MB)\n", valid_pct, valid_mb, max_mb,
				dirty_pct, dirty_mb, valid_mb);
	return 0;
}
