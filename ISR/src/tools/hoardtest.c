/*
 * hoardtest - Test program for hoard cache garbage collection
 *
 * Copyright (C) 2010 Carnegie Mellon University
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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include "sql.h"
#include "isrcrypto.h"

#define CHUNKSIZE 131072

static void __attribute__((noreturn)) die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

static void __attribute__((noreturn)) sql_die(struct db *db, const char *msg)
{
	sql_log_err(db, msg);
	die("SQL error");
}

static void __attribute__((noreturn)) usage(void)
{
	die("Usage: %s {create|dump} <hoard-cache>", g_get_prgname());
}

static void handle_log_message(const gchar *domain, GLogLevelFlags level,
			       const gchar *message, gpointer data)
{
	(void)domain;
	(void)data;

	switch (level & G_LOG_LEVEL_MASK) {
	case G_LOG_LEVEL_INFO:
	case SQL_LOG_LEVEL_QUERY:
	case SQL_LOG_LEVEL_SLOW_QUERY:
		break;
	default:
		fprintf(stderr, "%s\n", message);
		break;
	}
}

static void sha1(void *data, unsigned len, void *out)
{
	struct isrcry_hash_ctx *ctx;

	ctx = isrcry_hash_alloc(ISRCRY_HASH_SHA1);
	isrcry_hash_update(ctx, data, len);
	isrcry_hash_final(ctx, out);
	isrcry_hash_free(ctx);
}

static void create_hoard_index(struct db *db)
{
	if (!query(NULL, db, "PRAGMA user_version = 9", NULL))
		sql_die(db, "Couldn't set schema version");

	if (!query(NULL, db, "CREATE TABLE parcels ("
				"parcel INTEGER PRIMARY KEY NOT NULL, "
				"uuid TEXT UNIQUE NOT NULL, "
				"server TEXT NOT NULL, "
				"user TEXT NOT NULL, "
				"name TEXT NOT NULL)", NULL))
		sql_die(db, "Couldn't create parcel table");

	if (!query(NULL, db, "CREATE TABLE chunks ("
				"tag BLOB UNIQUE, "
				/* 512-byte sectors */
				"offset INTEGER UNIQUE NOT NULL, "
				"length INTEGER NOT NULL DEFAULT 0, "
				"crypto INTEGER NOT NULL DEFAULT 0, "
				"allocated INTEGER NOT NULL DEFAULT 0)",
				NULL))
		sql_die(db, "Couldn't create chunk table");
	if (!query(NULL, db, "CREATE INDEX chunks_allocated ON "
				"chunks (allocated, offset)", NULL))
		sql_die(db, "Couldn't create chunk allocation index");

	if (!query(NULL, db, "CREATE TABLE refs ("
				"parcel INTEGER NOT NULL, "
				"tag BLOB NOT NULL)", NULL))
		sql_die(db, "Couldn't create reference table");
	if (!query(NULL, db, "CREATE UNIQUE INDEX refs_constraint "
				"ON refs (parcel, tag)", NULL))
		sql_die(db, "Couldn't create chunk LRU index");
	if (!query(NULL, db, "CREATE INDEX refs_bytag ON refs "
				"(tag, parcel)", NULL))
		sql_die(db, "Couldn't create chunk reverse index");
}

static void do_create(struct db *db, int fd)
{
	unsigned offset = 0;
	char buf[CHUNKSIZE + 32];
	size_t len;
	char hash[20];

	create_hoard_index(db);
	if (!query(NULL, db, "INSERT INTO parcels (parcel, uuid, server, "
				"user, name) VALUES (1, "
				"'00000000-0000-0000-0000-000000000000', "
				"'server', 'user', 'name')", NULL))
		sql_die(db, "Couldn't insert parcel record");

	while (fgets(buf, sizeof(buf), stdin)) {
		g_strstrip(buf);
		len = strlen(buf);
		if (len == 0 || buf[0] == '#') {
			continue;
		} else if (buf[0] == '>') {
			if (len < 3)
				die("Short read from stdin");
			if (pwrite(fd, buf + 3, len - 3, ((off_t) offset) << 9)
						!= (ssize_t) (len - 3))
				die("Short write to hoard file");
			sha1(buf + 3, len - 3, hash);
			if (!query(NULL, db, "INSERT INTO chunks (tag, "
					"offset, length, crypto, allocated) "
					"VALUES (?, ?, ?, 2, 1)", "bdd",
					hash, sizeof(hash), offset, len - 3))
				sql_die(db, "Couldn't insert chunk record");
			if (buf[1] == 'r')
				if (!query(NULL, db, "INSERT INTO refs "
						"(parcel, tag) VALUES (1, ?)",
						"b", hash, sizeof(hash)))
					sql_die(db, "Couldn't insert "
								"chunk ref");
		} else {
			if (pwrite(fd, buf, 1, ((off_t) offset) << 9) != 1)
				die("Short write to hoard file");
			if (!query(NULL, db, "INSERT INTO chunks (offset, "
					"allocated) VALUES (?, ?)", "dd",
					offset, buf[0] == '_' ? 1 : 0))
				sql_die(db, "Couldn't insert empty slot");
		}
		offset += CHUNKSIZE >> 9;
	}
}

static void do_dump(struct db *db, int fd)
{
	struct query *qry;
	char buf[CHUNKSIZE];
	char hash[20];
	void *tag;
	unsigned taglen;
	unsigned offset;
	unsigned length;
	int allocated;
	int parcel;
	unsigned expected_offset = 0;
	struct stat st;

	for (query(&qry, db, "SELECT chunks.tag, offset, length, allocated, "
				"parcel FROM chunks LEFT JOIN refs "
				"ON chunks.tag == refs.tag ORDER BY offset",
				NULL); query_has_row(db); query_next(qry)) {
		query_row(qry, "bdddd", &tag, &taglen, &offset, &length,
					&allocated, &parcel);
		if (offset != expected_offset)
			die("Unexpected offset %u", offset);
		expected_offset += CHUNKSIZE >> 9;
		if (!allocated) {
			printf("..\n");
			continue;
		}
		if (allocated && taglen == 0) {
			printf("_.\n");
			continue;
		}
		if (taglen != sizeof(hash))
			die("Unexpected tag length %u", taglen);
		if (length > CHUNKSIZE)
			die("Unexpected chunk length %u", length);
		if (pread(fd, buf, length, ((off_t) offset) << 9) !=
					(ssize_t) length)
			die("Short read from cache file");
		sha1(buf, length, hash);
		if (memcmp(hash, tag, taglen))
			die("Tag check failed at offset %u", offset);
		printf(">%s %.*s\n", parcel == 1 ? "r" : ".", length, buf);
	}
	query_free(qry);
	if (!query_ok(db))
		sql_die(db, "Query failed");
	if (fstat(fd, &st))
		die("Couldn't stat hoard file");
	if (st.st_size < (((off_t) expected_offset) << 9) - CHUNKSIZE + 1 ||
				st.st_size > (((off_t) expected_offset) << 9))
		die("Unexpected hoard file size");
}

int main(int argc, char **argv)
{
	struct db *db;
	int hoard_fd;
	const char *mode;
	const char *dir;
	gchar *file;
	gboolean create;

	/* Parse arguments */
	g_set_prgname(argv[0]);
	if (argc != 3)
		usage();
	mode = argv[1];
	dir = argv[2];
	if (!strcmp(mode, "create"))
		create = TRUE;
	else if (!strcmp(mode, "dump"))
		create = FALSE;
	else
		usage();

	/* Initialize SQLite */
	g_log_set_handler("isrsql", G_LOG_LEVEL_MASK, handle_log_message,
				NULL);
	sql_init();

	/* Create hoard directory */
	if (create)
		if (!g_file_test(dir, G_FILE_TEST_IS_DIR) && mkdir(dir, 0755))
			die("Couldn't create hoard cache %s", dir);

	/* Open hoard file */
	file = g_strdup_printf("%s/hoard", dir);
	if (create)
		unlink(file);
	hoard_fd = open(file, O_RDWR | O_CREAT, 0755);
	if (hoard_fd == -1)
		die("Couldn't open hoard file %s", file);
	g_free(file);

	/* Open hoard index */
	file = g_strdup_printf("%s/hoard.idx", dir);
	if (create)
		unlink(file);
	if (!sql_conn_open(file, &db))
		die("Couldn't open database %s", file);
	g_free(file);

	/* Start transaction */
	if (!begin(db))
		die("Couldn't start transaction");

	/* Run operation */
	if (create)
		do_create(db, hoard_fd);
	else
		do_dump(db, hoard_fd);

	/* Shut down */
	if (!commit(db))
		die("Couldn't commit transaction");
	sql_conn_close(db);
	close(hoard_fd);
	return 0;
}
