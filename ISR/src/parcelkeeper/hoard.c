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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "defs.h"

#define HOARD_INDEX_VERSION 9
#define EXPAND_CHUNKS 256

/* Generator for an transaction wrapper around a function which expects to be
   called within a state->hoard transaction.  The wrapper discards errors
   and must be defined to return void. */
#define TRANSACTION_WRAPPER				\
TRANSACTION_DECL					\
{							\
	gboolean retry;					\
again:							\
	if (!begin(state->hoard))			\
		return;					\
	if (TRANSACTION_CALL) {				\
		retry = query_busy(state->hoard);	\
		rollback(state->hoard);			\
		if (retry) {				\
			query_backoff(state->hoard);	\
			goto again;			\
		}					\
		return;					\
	}						\
	if (!commit(state->hoard))			\
		rollback(state->hoard);			\
}

static pk_err_t create_hoard_index(struct pk_state *state)
{
	if (!query(NULL, state->hoard, "PRAGMA user_version = "
				G_STRINGIFY(HOARD_INDEX_VERSION), NULL)) {
		sql_log_err(state->hoard, "Couldn't set schema version");
		return PK_IOERR;
	}

	if (!query(NULL, state->hoard, "CREATE TABLE parcels ("
				"parcel INTEGER PRIMARY KEY NOT NULL, "
				"uuid TEXT UNIQUE NOT NULL, "
				"server TEXT NOT NULL, "
				"user TEXT NOT NULL, "
				"name TEXT NOT NULL)", NULL)) {
		sql_log_err(state->hoard, "Couldn't create parcel table");
		return PK_IOERR;
	}

	if (!query(NULL, state->hoard, "CREATE TABLE chunks ("
				"tag BLOB UNIQUE, "
				/* 512-byte sectors */
				"offset INTEGER UNIQUE NOT NULL, "
				"length INTEGER NOT NULL DEFAULT 0, "
				"crypto INTEGER NOT NULL DEFAULT 0, "
				"allocated INTEGER NOT NULL DEFAULT 0)",
				NULL)) {
		sql_log_err(state->hoard, "Couldn't create chunk table");
		return PK_IOERR;
	}
	if (!query(NULL, state->hoard, "CREATE INDEX chunks_allocated ON "
				"chunks (allocated, offset)", NULL)) {
		sql_log_err(state->hoard, "Couldn't create chunk allocation "
					"index");
		return PK_IOERR;
	}

	if (!query(NULL, state->hoard, "CREATE TABLE refs ("
				"parcel INTEGER NOT NULL, "
				"tag BLOB NOT NULL)", NULL)) {
		sql_log_err(state->hoard, "Couldn't create reference table");
		return PK_IOERR;
	}
	if (!query(NULL, state->hoard, "CREATE UNIQUE INDEX refs_constraint "
				"ON refs (parcel, tag)", NULL)) {
		sql_log_err(state->hoard, "Couldn't create chunk LRU index");
		return PK_IOERR;
	}
	if (!query(NULL, state->hoard, "CREATE INDEX refs_bytag ON refs "
				"(tag, parcel)", NULL)) {
		sql_log_err(state->hoard, "Couldn't create chunk reverse "
					"index");
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

static pk_err_t upgrade_hoard_index(struct pk_state *state, int ver)
{
	pk_log(LOG_INFO, "Upgrading hoard cache version %d to version %d",
				ver, HOARD_INDEX_VERSION);
	switch (ver) {
	default:
		pk_log(LOG_ERROR, "Unrecognized hoard cache version %d, "
					"bailing out", ver);
		return PK_BADFORMAT;
	case 5:
		if (!query(NULL, state->hoard, "DROP INDEX chunks_lru", NULL)) {
			sql_log_err(state->hoard, "Couldn't drop old chunk "
						"LRU index");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "CREATE INDEX chunks_lru ON "
					"chunks (referenced, last_access)",
					NULL)) {
			sql_log_err(state->hoard, "Couldn't create new "
						"chunk LRU index");
			return PK_IOERR;
		}
		/* Fall through */
	case 6:
		if (!query(NULL, state->hoard, "CREATE INDEX refs_bytag ON refs "
					"(tag, parcel)", NULL)) {
			sql_log_err(state->hoard, "Couldn't create chunk "
						"reverse index");
			return PK_IOERR;
		}
		/* Fall through */
	case 7:
		/* We can't rename/delete columns, so we have to recreate the
		   table */
		if (!query(NULL, state->hoard, "ALTER TABLE chunks "
				"RENAME TO chunks_old", NULL)) {
			sql_log_err(state->hoard, "Couldn't rename chunks "
						"table");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "CREATE TABLE chunks ("
				"tag BLOB UNIQUE, "
				"offset INTEGER UNIQUE NOT NULL, "
				"length INTEGER NOT NULL DEFAULT 0, "
				"crypto INTEGER NOT NULL DEFAULT 0, "
				"allocated INTEGER NOT NULL DEFAULT 0)",
				NULL)) {
			sql_log_err(state->hoard, "Couldn't create new "
						"chunks table");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "INSERT INTO chunks "
				"(tag, offset, length, crypto, allocated) "
				"SELECT tag, offset, length, crypto, "
				"referenced FROM chunks_old", NULL)) {
			sql_log_err(state->hoard, "Couldn't update chunks "
						"table");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "CREATE INDEX chunks_allocated "
				"ON chunks (allocated)", NULL)) {
			sql_log_err(state->hoard, "Couldn't create allocated "
						"index");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "DROP TABLE chunks_old", NULL)) {
			sql_log_err(state->hoard, "Couldn't drop old chunks "
						"table");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "UPDATE chunks SET tag = NULL, "
				"length = 0, crypto = 0 WHERE allocated == 0 "
				"AND tag NOTNULL", NULL)) {
			sql_log_err(state->hoard, "Couldn't clear "
						"unreferenced chunks");
			return PK_IOERR;
		}
		/* Fall through */
	case 8:
		if (!query(NULL, state->hoard, "DROP INDEX chunks_allocated",
					NULL)) {
			sql_log_err(state->hoard, "Couldn't drop old "
						"allocated chunk index");
			return PK_IOERR;
		}
		if (!query(NULL, state->hoard, "CREATE INDEX chunks_allocated "
					"ON chunks (allocated, offset)",
					NULL)) {
			sql_log_err(state->hoard, "Couldn't create new "
						"allocated chunk index");
			return PK_IOERR;
		}
	}
	if (!query(NULL, state->hoard, "PRAGMA user_version = "
				G_STRINGIFY(HOARD_INDEX_VERSION), NULL)) {
		sql_log_err(state->hoard, "Couldn't update schema version");
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

static pk_err_t create_slot_cache(struct pk_state *state)
{
	if (!query(NULL, state->hoard, "CREATE TEMP TABLE slots ("
				"tag BLOB UNIQUE, "
				"offset INTEGER UNIQUE NOT NULL, "
				"length INTEGER NOT NULL DEFAULT 0, "
				"crypto INTEGER NOT NULL DEFAULT 0)",
				NULL)) {
		sql_log_err(state->hoard, "Couldn't create slot cache");
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

/* XXX cache chunks of different sizes */
/* must be within transaction */
static pk_err_t expand_slot_cache(struct pk_state *state)
{
	struct query *qry;
	int count;
	int start;
	int i;
	int step = state->parcel->chunksize >> 9;
	int needed=EXPAND_CHUNKS;
	gchar *type;

	/* First, try to use existing unallocated slots */
	if (!query(&qry, state->hoard, "INSERT OR IGNORE INTO temp.slots "
				"(offset) SELECT offset FROM chunks "
				"WHERE allocated == 0 LIMIT ?", "d", needed)) {
		sql_log_err(state->hoard, "Error reclaiming hoard cache slots");
		return PK_IOERR;
	}
	query_row(qry, "d", &count);
	query_free(qry);
	needed -= count;
	if (!query(NULL, state->hoard, "UPDATE chunks SET allocated = 1 "
				"WHERE offset IN "
				"(SELECT offset FROM temp.slots)", NULL)) {
		sql_log_err(state->hoard, "Couldn't allocate chunk slots");
		return PK_IOERR;
	}
	if (needed == 0)
		return PK_SUCCESS;

	/* Now expand the hoard cache as necessary to meet our quota */
	query(&qry, state->hoard, "SELECT max(offset) FROM chunks", NULL);
	if (!query_has_row(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't find max hoard cache "
					"offset");
		return PK_IOERR;
	}
	/* Distinguish between a max offset of 0 and a NULL value (indicating
	   that the table is empty) */
	type = query_column_types(qry);
	query_row(qry, "d", &start);
	query_free(qry);
	if (strcmp(type, "0"))
		start += step;
	g_free(type);
	for (i=0; i<needed; i++) {
		if (!query(NULL, state->hoard, "INSERT INTO temp.slots "
					"(offset) VALUES (?)", "d",
					start + i * step)) {
			sql_log_err(state->hoard, "Couldn't add new offset %d "
						"to slot cache",
						start + i * step);
			return PK_IOERR;
		}
	}
	if (!query(NULL, state->hoard, "INSERT OR IGNORE INTO chunks "
				"(offset, allocated) "
				"SELECT offset, 1 FROM temp.slots",
				NULL)) {
		sql_log_err(state->hoard, "Couldn't expand hoard cache");
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

/* Must be within transaction.  Does not add chunk references. */
static pk_err_t _flush_slot_cache(struct pk_state *state)
{
	struct query *qry;
	pk_err_t ret;
	void *tag;
	int taglen;
	int offset;
	int len;
	int crypto;

	for (query(&qry, state->hoard, "SELECT tag, offset, length, crypto "
				"FROM temp.slots WHERE tag NOTNULL", NULL);
				query_has_row(state->hoard); query_next(qry)) {
		query_row(qry, "bddd", &tag, &taglen, &offset, &len, &crypto);
		query(NULL, state->hoard, "UPDATE chunks SET tag = ?, "
					"length = ?, crypto = ?, "
					"allocated = 1 WHERE offset = ?",
					"bddd", tag, taglen, len, crypto,
					offset);

		if (query_constrained(state->hoard)) {
			if (!query(NULL, state->hoard, "UPDATE chunks "
						"SET allocated = 0 WHERE "
						"offset == ?", "d", offset)) {
				sql_log_err(state->hoard, "Couldn't release "
							"allocation on offset "
							"%d", offset);
				ret=PK_IOERR;
				goto bad;
			}
		} else if (!query_has_row(state->hoard)) {
			sql_log_err(state->hoard, "Couldn't update chunks "
						"table for offset %d", offset);
			ret=PK_IOERR;
			goto bad;
		}
	}
	query_free(qry);
	if (!query_ok(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't query slot cache");
		return PK_IOERR;
	}

	if (!query(NULL, state->hoard, "UPDATE chunks SET allocated = 0 WHERE "
				"offset IN (SELECT offset FROM temp.slots "
				"WHERE tag ISNULL)", NULL)) {
		sql_log_err(state->hoard, "Couldn't free unused cache slots");
		return PK_IOERR;
	}
	if (!query(NULL, state->hoard, "DELETE FROM temp.slots", NULL)) {
		sql_log_err(state->hoard, "Couldn't clear slot cache");
		return PK_IOERR;
	}
	return PK_SUCCESS;

bad:
	query_free(qry);
	return ret;
}

#define TRANSACTION_DECL	static void flush_slot_cache( \
						struct pk_state *state)
#define TRANSACTION_CALL	_flush_slot_cache(state)
TRANSACTION_WRAPPER
#undef TRANSACTION_DECL
#undef TRANSACTION_CALL

/* must be within transaction */
static pk_err_t allocate_slot(struct pk_state *state, int *offset)
{
	struct query *qry;
	pk_err_t ret;

	while (1) {
		/* First, try to find an unused slot in the slot cache */
		query(&qry, state->hoard, "SELECT offset FROM temp.slots "
					"WHERE tag ISNULL LIMIT 1", NULL);
		if (query_has_row(state->hoard)) {
			query_row(qry, "d", offset);
			query_free(qry);
			break;
		} else if (!query_ok(state->hoard)) {
			sql_log_err(state->hoard, "Error finding unused "
						"hoard cache slot");
			return PK_IOERR;
		}

		/* There aren't any, so we have some work to do.  First,
		   flush the existing slot cache back to the chunks table. */
		ret=_flush_slot_cache(state);
		if (ret)
			return ret;

		/* Now populate the slot cache and try again. */
		ret=expand_slot_cache(state);
		if (ret)
			return ret;
	}
	return PK_SUCCESS;
}

/* This function is intended to be used when a particular chunk in the hoard
   cache is found to be invalid (e.g., the data does not match the tag).
   It first checks to make sure that the provided tag/offset pair is still
   valid, in case the chunk in the hoard cache was deleted out from under us
   as we were reading it.  (hoard_get_chunk() cares about this case.)
   Must be called within transaction for hoard connection. */
static pk_err_t _hoard_invalidate_chunk(struct pk_state *state, int offset,
			const void *tag, unsigned taglen)
{
	struct query *qry;

	query(&qry, state->hoard, "SELECT offset FROM chunks WHERE "
				"offset == ? AND tag == ?", "db",
				offset, tag, taglen);
	if (query_ok(state->hoard)) {
		/* It's already not there. */
		return PK_SUCCESS;
	} else if (!query_has_row(state->hoard)) {
		sql_log_err(state->hoard, "Could not query chunk list");
		return PK_IOERR;
	}
	query_free(qry);

	if (!query(NULL, state->hoard, "UPDATE chunks SET tag = NULL, "
				"length = 0, crypto = 0, allocated = 0 "
				"WHERE offset = ?", "d", offset)) {
		sql_log_err(state->hoard, "Couldn't deallocate hoard chunk "
					"at offset %d", offset);
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

/* Same as _hoard_invalidate_chunk(), but for the slot cache.  We don't
   need to check that the row being deleted is still valid, since there's no
   contention for the slot cache. */
static pk_err_t _hoard_invalidate_slot_chunk(struct pk_state *state,
			int offset)
{
	if (!query(NULL, state->hoard, "UPDATE temp.slots SET tag = NULL, "
				"length = 0, crypto = 0 WHERE offset = ?",
				"d", offset)) {
		sql_log_err(state->hoard, "Couldn't deallocate hoard slot "
					"at offset %d", offset);
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

#define TRANSACTION_DECL	void hoard_invalidate_chunk( \
					struct pk_state *state, int offset, \
					const void *tag, unsigned taglen)
#define TRANSACTION_CALL	_hoard_invalidate_chunk(state, offset, tag, \
					taglen)
TRANSACTION_WRAPPER
#undef TRANSACTION_DECL
#undef TRANSACTION_CALL

#define TRANSACTION_DECL	static void hoard_invalidate_slot_chunk( \
					struct pk_state *state, int offset)
#define TRANSACTION_CALL	_hoard_invalidate_slot_chunk(state, offset)
TRANSACTION_WRAPPER
#undef TRANSACTION_DECL
#undef TRANSACTION_CALL

pk_err_t _hoard_read_chunk(struct pk_state *state, int offset, int length,
			int crypto, const void *tag, int taglen, void *buf)
{
	unsigned hashlen = iu_chunk_crypto_hashlen(crypto);
	char calctag[hashlen];

	/* Check expected tag length */
	if (taglen != (int) hashlen) {
		pk_log(LOG_WARNING, "Hoard chunk has incorrect tag length %d "
					"(expected %d)", taglen, hashlen);
		return PK_BADFORMAT;
	}

	/* Check chunk offset and length */
	if (offset < 0 || length <= 0 || (state->parcel != NULL &&
				(unsigned)length > state->parcel->chunksize)) {
		pk_log(LOG_WARNING, "Hoard chunk has unreasonable "
					"offset/length %d/%d", offset, length);
		return PK_BADFORMAT;
	}

	/* Read the data */
	if (pread(state->hoard_fd, buf, length, ((off_t)offset) << 9)
				!= length) {
		pk_log(LOG_WARNING, "Couldn't read hoard chunk at offset %d",
					offset);
		return PK_IOERR;
	}

	/* Check its hash */
	if (!iu_chunk_crypto_digest(crypto, calctag, buf, length))
		return PK_CALLFAIL;
	if (memcmp(tag, calctag, hashlen)) {
		pk_log(LOG_WARNING, "Tag mismatch reading hoard cache at "
					"offset %d", offset);
		log_tag_mismatch(tag, calctag, hashlen);
		return PK_TAGFAIL;
	}
	return PK_SUCCESS;
}

pk_err_t hoard_get_chunk(struct pk_state *state, const void *tag, void *buf,
			unsigned *len)
{
	struct query *qry;
	int offset;
	int clen;
	pk_err_t ret;
	gboolean slot_cache;
	gboolean retry;

	if (state->conf->hoard_dir == NULL)
		return PK_NOTFOUND;

again:
	if (!begin(state->hoard))
		return PK_IOERR;

	/* First query the slot cache */
	if (!query(&qry, state->hoard, "SELECT offset, length FROM temp.slots "
				"WHERE tag == ?", "b", tag,
				state->parcel->hashlen)) {
		sql_log_err(state->hoard, "Couldn't query slot cache");
		ret=PK_IOERR;
		goto bad;
	}
	if (query_has_row(state->hoard)) {
		query_row(qry, "dd", &offset, &clen);
		query_free(qry);
		slot_cache = TRUE;
	} else {
		/* Now query the hoard cache */
		slot_cache = FALSE;
		query(&qry, state->hoard, "SELECT offset, length FROM chunks "
					"WHERE tag == ?", "b", tag,
					state->parcel->hashlen);
		if (query_ok(state->hoard)) {
			if (!commit(state->hoard)) {
				ret=PK_IOERR;
				goto bad;
			}
			return PK_NOTFOUND;
		} else if (!query_has_row(state->hoard)) {
			sql_log_err(state->hoard, "Couldn't query hoard "
						"chunk index");
			ret=PK_IOERR;
			goto bad;
		}
		query_row(qry, "dd", &offset, &clen);
		query_free(qry);
	}

	if (!commit(state->hoard)) {
		ret=PK_IOERR;
		goto bad;
	}

	if (_hoard_read_chunk(state, offset, clen, state->parcel->crypto,
				tag, state->parcel->hashlen, buf)) {
		/* Read failures can occur if the chunk has been moved due
		   to GC compaction, so we don't want to blindly invalidate
		   the slot in case some other data has been stored there
		   in the interim.  Therefore, _hoard_invalidate_chunk()
		   checks that the tag/index pair is still present in the
		   chunks table before invalidating the slot.  If we're
		   working from the slot cache, this race does not apply. */
		pk_log(LOG_ERROR, "Invalidating chunk and retrying");
		if (slot_cache)
			hoard_invalidate_slot_chunk(state, offset);
		else
			hoard_invalidate_chunk(state, offset, tag,
						state->parcel->hashlen);
		/* GC could have moved the chunk, try again */
		goto again;
	}
	*len=clen;
	return PK_SUCCESS;

bad:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
	return ret;
}

/* Writing out only the data bytes in @buf causes a read-modify-write
   in the kernel's page cache if the length is not a multiple of 512
   (or 4096?), reducing throughput.  We could pad out to the next 4096-byte
   boundary, but instead we pad to the entire length of the slot, based on
   two assumptions: the extra I/O is cheap, and keeping the hoard cache
   from becoming sparse is useful to avoid filesystem fragmentation when
   chunks are overwritten by larger chunks. */
pk_err_t _hoard_write_chunk(struct pk_state *state, unsigned offset,
			unsigned length, const void *buf)
{
	unsigned chunksize = 131072;  /* XXX */
	char data[chunksize];

	if (length > chunksize)
		return PK_INVALID;
	memcpy(data, buf, length);
	memset(data + length, 0, chunksize - length);
	if (pwrite(state->hoard_fd, data, chunksize, ((off_t)offset) << 9) !=
				(int) chunksize) {
		pk_log(LOG_ERROR, "Couldn't write hoard cache at offset %d",
					offset);
		return PK_IOERR;
	}
	return PK_SUCCESS;
}

pk_err_t hoard_put_chunk(struct pk_state *state, const void *tag,
			const void *buf, unsigned len)
{
	pk_err_t ret;
	int offset;
	gboolean retry;

	if (state->conf->hoard_dir == NULL)
		return PK_SUCCESS;

again:
	if (!begin(state->hoard))
		return PK_IOERR;

	/* See if the tag is already in the slot cache */
	query(NULL, state->hoard, "SELECT tag FROM temp.slots WHERE tag == ?",
				"b", tag, state->parcel->hashlen);
	if (query_has_row(state->hoard)) {
		if (!commit(state->hoard)) {
			ret=PK_IOERR;
			goto bad;
		}
		return PK_SUCCESS;
	} else if (!query_ok(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't look up tag in slot cache");
		ret=PK_SQLERR;
		goto bad;
	}

	/* See if the tag is already in the hoard cache */
	query(NULL, state->hoard, "SELECT tag FROM chunks WHERE tag == ?",
				"b", tag, state->parcel->hashlen);
	if (query_has_row(state->hoard)) {
		if (!commit(state->hoard)) {
			ret=PK_IOERR;
			goto bad;
		}
		return PK_SUCCESS;
	} else if (!query_ok(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't look up tag in hoard "
					"cache index");
		ret=PK_SQLERR;
		goto bad;
	}

	ret=allocate_slot(state, &offset);
	if (ret)
		goto bad;
	if (!query(NULL, state->hoard, "UPDATE temp.slots SET tag = ?, "
				"length = ?, crypto = ? WHERE offset = ?",
				"bddd", tag, state->parcel->hashlen, len,
				state->parcel->crypto, offset)) {
		sql_log_err(state->hoard, "Couldn't add metadata for hoard "
					"cache chunk");
		ret=PK_IOERR;
		goto bad;
	}
	if (_hoard_write_chunk(state, offset, len, buf)) {
		ret=PK_IOERR;
		goto bad;
	}
	if (!commit(state->hoard)) {
		pk_log(LOG_ERROR, "Couldn't commit hoard cache chunk");
		ret=PK_IOERR;
		goto bad;
	}
	return PK_SUCCESS;

bad:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
	return ret;
}

/* We use state->db rather than state->hoard in this function, since we need to
   compare to the previous or current keyring */
pk_err_t hoard_sync_refs(struct pk_state *state, gboolean new_chunks)
{
	gboolean retry;

	if (state->conf->hoard_dir == NULL)
		return PK_SUCCESS;

again:
	if (!begin_immediate(state->db))
		return PK_IOERR;
	if (new_chunks)
		query(NULL, state->db, "CREATE TEMP TABLE newrefs AS "
					"SELECT DISTINCT tag FROM keys", NULL);
	else
		query(NULL, state->db, "CREATE TEMP TABLE newrefs AS "
					"SELECT DISTINCT tag FROM prev.keys",
					NULL);
	if (!query_ok(state->db)) {
		sql_log_err(state->db, "Couldn't generate tag list");
		goto bad;
	}
	if (!query(NULL, state->db, "CREATE INDEX temp.newrefs_tags ON "
				"newrefs (tag)", NULL)) {
		sql_log_err(state->db, "Couldn't create tag index");
		goto bad;
	}
	if (!new_chunks) {
		if (!query(NULL, state->db, "DELETE FROM hoard.refs WHERE "
					"parcel == ? AND tag NOT IN "
					"(SELECT tag FROM temp.newrefs)",
					"d", state->hoard_ident)) {
			sql_log_err(state->db, "Couldn't garbage-collect "
						"hoard refs");
			goto bad;
		}
	}
	if (!query(NULL, state->db, "INSERT OR IGNORE INTO hoard.refs "
				"(parcel, tag) SELECT ?, tag FROM temp.newrefs",
				"d", state->hoard_ident)) {
		sql_log_err(state->db, "Couldn't insert new hoard refs");
		goto bad;
	}
	if (!query(NULL, state->db, "DROP TABLE temp.newrefs", NULL)) {
		sql_log_err(state->db, "Couldn't drop temporary table");
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

static pk_err_t get_parcel_ident(struct pk_state *state)
{
	struct query *qry;
	gboolean retry;

again:
	if (!begin(state->hoard))
		return PK_IOERR;
	/* Add the row if it's not already there */
	if (!query(NULL, state->hoard, "INSERT OR IGNORE INTO parcels "
				"(uuid, server, user, name) "
				"VALUES (?, ?, ?, ?)", "SSSS",
				state->parcel->uuid, state->parcel->server,
				state->parcel->user, state->parcel->parcel)) {
		sql_log_err(state->hoard, "Couldn't insert parcel record");
		goto bad;
	}
	/* Find out the parcel ID assigned by SQLite */
	query(&qry, state->hoard, "SELECT parcel FROM parcels WHERE uuid == ?",
				"S", state->parcel->uuid);
	if (!query_has_row(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't query parcels table");
		goto bad;
	}
	query_row(qry, "d", &state->hoard_ident);
	query_free(qry);
	/* Make sure the row has current metadata in case it was already
	   present.  Don't promote the lock if no update is necessary. */
	if (!query(NULL, state->hoard, "UPDATE parcels SET server = ?, "
				"user = ?, name = ? WHERE parcel == ? AND "
				"(server != ? OR user != ? OR name != ?)",
				"SSSdSSS", state->parcel->server,
				state->parcel->user, state->parcel->parcel,
				state->hoard_ident, state->parcel->server,
				state->parcel->user, state->parcel->parcel)) {
		sql_log_err(state->hoard, "Couldn't update parcel record");
		goto bad;
	}
	if (!commit(state->hoard))
		goto bad;
	return PK_SUCCESS;

bad:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
	return PK_IOERR;
}

static pk_err_t open_hoard_index(struct pk_state *state)
{
	struct query *qry;
	pk_err_t ret;
	int ver;
	gboolean retry;

	/* First open the dedicated hoard cache DB connection */
	if (!sql_conn_open(state->conf->hoard_index, &state->hoard))
		return PK_IOERR;

again:
	if (!begin(state->hoard)) {
		ret=PK_IOERR;
		goto bad;
	}
	query(&qry, state->hoard, "PRAGMA user_version", NULL);
	if (!query_has_row(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't get hoard cache index "
					"version");
		ret=PK_IOERR;
		goto bad_rollback;
	}
	query_row(qry, "d", &ver);
	query_free(qry);
	ret=PK_SUCCESS;
	if (ver == 0) {
		ret=create_hoard_index(state);
	} else if (ver < HOARD_INDEX_VERSION) {
		ret=upgrade_hoard_index(state, ver);
	} else if (ver > HOARD_INDEX_VERSION) {
		pk_log(LOG_ERROR, "Hoard cache version %d too new (expected "
					"%d)", ver, HOARD_INDEX_VERSION);
		ret=PK_BADFORMAT;
	}
	if (ret)
		goto bad_rollback;
	ret=create_slot_cache(state);
	if (ret)
		goto bad_rollback;
	if (!commit(state->hoard)) {
		ret=PK_IOERR;
		goto bad_rollback;
	}

	/* Now attach the hoard cache index to the primary DB connection
	   for cross-table queries */
	if (!attach(state->db, "hoard", state->conf->hoard_index)) {
		ret=PK_IOERR;
		goto bad;
	}
	interrupter_add(state->hoard);
	return PK_SUCCESS;

bad_rollback:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
bad:
	sql_conn_close(state->hoard);
	return ret;
}

/* Must be in hoard transaction */
static pk_err_t _hoard_gc(struct pk_state *state)
{
	return cleanup_action(state->hoard, "UPDATE chunks SET tag = NULL, "
				"length = 0, crypto = 0, allocated = 0 "
				"WHERE tag NOTNULL AND tag NOT IN "
				"(SELECT tag FROM refs)",
				LOG_INFO, "unreferenced chunks");
}

pk_err_t hoard_gc(struct pk_state *state)
{
	gboolean retry;

again:
	if (!begin(state->hoard))
		return PK_IOERR;
	if (_hoard_gc(state))
		goto bad;
	if (!commit(state->hoard))
		goto bad;
	return PK_SUCCESS;

bad:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
	return PK_IOERR;
}

/* Releases the hoard_fd lock before returning, including on error */
static pk_err_t hoard_try_cleanup(struct pk_state *state)
{
	struct query *qry;
	pk_err_t ret;
	int changes;
	int ident;
	gboolean retry;

	ret=get_file_lock(state->hoard_fd, FILE_LOCK_WRITE);
	if (ret == PK_BUSY) {
		pk_log(LOG_INFO, "Hoard cache in use; skipping cleanup");
		ret=PK_SUCCESS;
		goto out;
	} else if (ret) {
		goto out;
	}

	pk_log(LOG_INFO, "Cleaning up hoard cache...");
again:
	if (!begin(state->hoard)) {
		ret=PK_IOERR;
		goto out;
	}

	/* This was originally "DELETE FROM parcels WHERE parcel NOT IN
	   (SELECT DISTINCT parcel FROM refs)".  But the parcels table is
	   small and the refs table is large, and that query walked the entire
	   refs_constraint index.  Given the size of parcels table, the
	   below approach is much more efficient. */
	for (query(&qry, state->hoard, "SELECT parcel FROM parcels", NULL),
				changes=0; query_has_row(state->hoard);
				query_next(qry)) {
		query_row(qry, "d", &ident);
		if (!query(NULL, state->hoard, "SELECT parcel FROM refs WHERE "
					"parcel == ? LIMIT 1", "d", ident)) {
			sql_log_err(state->hoard, "Couldn't query refs table");
			query_free(qry);
			ret=PK_SQLERR;
			goto bad;
		}
		if (!query_has_row(state->hoard)) {
			if (!query(NULL, state->hoard, "DELETE FROM parcels "
						"WHERE parcel == ?", "d",
						ident)) {
				sql_log_err(state->hoard, "Couldn't delete "
						"unused parcel from hoard "
						"cache index");
				query_free(qry);
				ret=PK_SQLERR;
				goto bad;
			}
			changes++;
		}
	}
	query_free(qry);
	if (!query_ok(state->hoard)) {
		sql_log_err(state->hoard, "Couldn't query parcels table");
		ret=PK_SQLERR;
		goto bad;
	}
	if (changes > 0)
		pk_log(LOG_INFO, "Cleaned %d dangling parcel records",
					changes);

	ret=cleanup_action(state->hoard, "UPDATE chunks SET allocated = 0 "
				"WHERE allocated == 1 AND tag ISNULL",
				LOG_INFO, "orphaned cache slots");
	if (ret)
		goto bad;

	/* This query is slow when there are many refs, so perform it only
	   at refresh time and with 1/8 probability */
	if ((state->conf->flags & WANT_GC) && !g_random_int_range(0, 7)) {
		pk_log(LOG_INFO, "Garbage-collecting hoard cache");
		ret = _hoard_gc(state);
		if (ret)
			goto bad;
	}

	if (!commit(state->hoard)) {
		ret=PK_IOERR;
		goto bad;
	}

	/* Vacuum the hoard cache only during refresh and with 1/8
	   probability */
	if ((state->conf->flags & WANT_GC) && !g_random_int_range(0, 7)) {
		pk_log(LOG_INFO, "Vacuuming hoard cache");
		if (!vacuum(state->hoard)) {
			sql_log_err(state->hoard, "Couldn't vacuum "
						"hoard cache");
			/* There's not much to be done about this */
		}
	}

out:
	put_file_lock(state->hoard_fd);
	return ret;
bad:
	retry = query_busy(state->hoard);
	rollback(state->hoard);
	if (retry) {
		query_backoff(state->hoard);
		goto again;
	}
	goto out;
}

pk_err_t hoard_init(struct pk_state *state)
{
	pk_err_t ret;

	if (state->conf->hoard_dir == NULL)
		return PK_INVALID;
	if (state->parcel != NULL && state->parcel->chunksize != 131072) {
		pk_log(LOG_ERROR, "Hoard cache non-functional for chunk "
					"sizes != 128 KB");
		return PK_INVALID;
	}
	if (!g_file_test(state->conf->hoard_dir, G_FILE_TEST_IS_DIR) &&
				mkdir(state->conf->hoard_dir, 0777)) {
		pk_log(LOG_ERROR, "Couldn't create hoard directory %s",
					state->conf->hoard_dir);
		return PK_CALLFAIL;
	}

	state->hoard_fd=open(state->conf->hoard_file, O_RDWR|O_CREAT, 0666);
	if (state->hoard_fd == -1) {
		pk_log(LOG_ERROR, "Couldn't open %s", state->conf->hoard_file);
		return PK_IOERR;
	}
	ret=get_file_lock(state->hoard_fd, FILE_LOCK_READ|FILE_LOCK_WAIT);
	if (ret) {
		pk_log(LOG_ERROR, "Couldn't get read lock on %s",
					state->conf->hoard_file);
		goto bad;
	}

	ret=open_hoard_index(state);
	if (ret)
		goto bad;

	if (state->conf->parcel_dir != NULL) {
		ret=get_parcel_ident(state);
		if (ret)
			goto bad_close;
	}
	return PK_SUCCESS;

bad_close:
	sql_conn_close(state->hoard);
bad:
	close(state->hoard_fd);
	return ret;
}

void hoard_shutdown(struct pk_state *state)
{
	flush_slot_cache(state);
	hoard_try_cleanup(state);
	sql_conn_close(state->hoard);
	close(state->hoard_fd);
}
