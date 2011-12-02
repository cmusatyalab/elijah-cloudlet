/*
 * libisrsql - Wrapper code around a private version of SQLite
 *
 * Copyright (C) 2006-2009 Carnegie Mellon University
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

#define G_LOG_DOMAIN "isrsql"

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include "sqlite3.h"
#include "sql.h"

#define SLOW_THRESHOLD_MS 200
#define MAX_WAIT_USEC 10000
#define PROGRESS_HANDLER_INTERVAL 100000

struct db {
	pthread_mutex_t lock;
	pthread_t holder;
	sqlite3 *conn;
	unsigned queries;
	int result;  /* set by query() and query_next() */
	gchar *file;
	gchar *errmsg;
	gint interrupt;  /* glib atomic int operations */
	gboolean use_transaction;

	/* Statistics */
	unsigned busy_queries;
	unsigned busy_timeouts;
	unsigned retries;
	uint64_t wait_usecs;
};

struct query {
	struct db *db;
	sqlite3_stmt *stmt;
	const char *sql;
	GTimer *timer;
};

struct param {
	int index;
	char type;

	void *i_ptr;
	int64_t i_int;
	double i_float;

	const unsigned char **o_string;
	const void **o_data;
	int *o_int;
	int64_t *o_int64;
	double *o_float;
};

struct query_params {
	enum sql_param_type type;
	GList *list;
};

static void sqlerr(struct db *db, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	if (db->errmsg != NULL)
		g_free(db->errmsg);
	db->errmsg = g_strdup_vprintf(fmt, ap);
	va_end(ap);
}

static void db_get(struct db *db)
{
	pthread_mutex_lock(&db->lock);
	db->holder = pthread_self();
}

static void db_put(struct db *db)
{
	if (db->queries) {
		g_critical("Leaked %u queries", db->queries);
		db->queries = 0;
	}
	db->holder = (pthread_t) 0;
	pthread_mutex_unlock(&db->lock);
}

static gboolean db_in_trans(struct db *db)
{
	if (!pthread_mutex_trylock(&db->lock)) {
		pthread_mutex_unlock(&db->lock);
		return FALSE;
	}
	if (db->holder != pthread_self())
		return FALSE;
	return TRUE;
}

static void db_assert_trans(struct db *db)
{
	if (!db_in_trans(db))
		g_critical("Attempt to perform database operation outside a "
					"transaction");
}

static int alloc_query(struct query **new_qry, struct db *db, const char *sql)
{
	struct query *qry;
	int ret;

	qry=g_slice_new(struct query);
	qry->db=db;
	ret=sqlite3_prepare_v2(db->conn, sql, -1, &qry->stmt, NULL);
	if (ret) {
		sqlerr(db, "%s", sqlite3_errmsg(db->conn));
		g_slice_free(struct query, qry);
	} else {
		qry->sql=sqlite3_sql(qry->stmt);
		qry->timer=g_timer_new();
		db->queries++;
		*new_qry=qry;
	}
	return ret;
}

struct query_params *query_params_new(enum sql_param_type type)
{
	struct query_params *params;

	params = g_slice_new0(struct query_params);
	params->type = type;
	return params;
}

void query_params_free(struct query_params *params)
{
	while (params->list != NULL) {
		g_slice_free(struct param, params->list->data);
		params->list = g_list_delete_link(params->list, params->list);
	}
	g_slice_free(struct query_params, params);
}

static struct param *param_get(struct query_params *params, int index)
{
	GList *cur;
	struct param *param;

	for (cur = params->list; cur != NULL; cur = cur->next) {
		param = cur->data;
		if (param->index == index)
			return param;
	}
	param = g_slice_new0(struct param);
	param->index = index;
	params->list = g_list_prepend(params->list, param);
	return param;
}

static gboolean param_set_group(struct query_params *params, int start_idx,
			const char *types, va_list ap)
{
	struct param *param;
	int i;
	/* SQLite input parameters are 1-indexed */
	int offset = params->type == SQL_PARAM_INPUT ? 1 : 0;

	if (types == NULL)
		return TRUE;
	for (i = 0; types[i]; i++) {
		param = param_get(params, start_idx + i + offset);
		if (params->type == SQL_PARAM_INPUT) {
			switch (types[i]) {
			case 'd':
				param->i_int = va_arg(ap, int);
				break;
			case 'D':
				param->i_int = va_arg(ap, int64_t);
				break;
			case 'f':
				param->i_float = va_arg(ap, double);
				break;
			case 's':
			case 'S':
				param->i_ptr = va_arg(ap, char *);
				break;
			case 'b':
			case 'B':
				param->i_ptr = va_arg(ap, void *);
				param->i_int = va_arg(ap, int);
				break;
			default:
				g_critical("Unknown format specifier %c",
							types[i]);
				return FALSE;
			}
		} else {
			switch (types[i]) {
			case 'd':
			case 'n':
				param->o_int = va_arg(ap, int *);
				break;
			case 'D':
				param->o_int64 = va_arg(ap, int64_t *);
				break;
			case 'f':
				param->o_float = va_arg(ap, double *);
				break;
			case 's':
				param->o_string = va_arg(ap,
							const unsigned char **);
				break;
			case 'S':
				param->o_string = va_arg(ap,
							const unsigned char **);
				param->o_int = va_arg(ap, int *);
				break;
			case 'b':
				param->o_data = va_arg(ap, const void **);
				param->o_int = va_arg(ap, int *);
				break;
			default:
				g_critical("Unknown format specifier %c",
							types[i]);
				return FALSE;
			}
		}
		param->type = types[i];
	}
	return TRUE;
}

gboolean query_param_set(struct query_params *params, int index, char type, ...)
{
	char types[] = {type, 0};
	va_list ap;
	gboolean ret;

	va_start(ap, type);
	ret = param_set_group(params, index, types, ap);
	va_end(ap);
	return ret;
}

gboolean query_v(struct query **new_qry, struct db *db, const char *query,
			struct query_params *params)
{
	struct query *qry;
	struct param *param;
	sqlite3_stmt *stmt;
	GList *cur;

	if (new_qry != NULL)
		*new_qry=NULL;
	if (!db_in_trans(db)) {
		db->result=SQLITE_MISUSE;
		sqlerr(db, "Attempt to perform database operation outside "
					"a transaction");
		return FALSE;
	}
	if (params->type != SQL_PARAM_INPUT) {
		db->result = SQLITE_MISUSE;
		sqlerr(db, "Query parameters are not of type input");
		return FALSE;
	}
	db->result=alloc_query(&qry, db, query);
	if (db->result)
		return FALSE;
	stmt=qry->stmt;
	for (cur = params->list; cur != NULL; cur = cur->next) {
		param = cur->data;
		switch (param->type) {
		case 'd':
		case 'D':
			db->result = sqlite3_bind_int64(stmt, param->index,
						param->i_int);
			break;
		case 'f':
			db->result = sqlite3_bind_double(stmt, param->index,
						param->i_float);
			break;
		case 's':
		case 'S':
			db->result = sqlite3_bind_text(stmt, param->index,
						param->i_ptr, -1,
						param->type == 's'
						? SQLITE_TRANSIENT
						: SQLITE_STATIC);
			break;
		case 'b':
		case 'B':
			db->result = sqlite3_bind_blob(stmt, param->index,
						param->i_ptr, param->i_int,
						param->type == 'b'
						? SQLITE_TRANSIENT
						: SQLITE_STATIC);
			break;
		default:
			g_assert_not_reached();
			break;
		}
		if (db->result)
			break;
	}
	if (db->result == SQLITE_OK)
		query_next(qry);
	else
		sqlerr(db, "%s", sqlite3_errmsg(db->conn));
	if (db->result != SQLITE_ROW || new_qry == NULL)
		query_free(qry);
	else
		*new_qry=qry;
	if (db->result == SQLITE_OK || db->result == SQLITE_ROW)
		return TRUE;
	else
		return FALSE;
}

gboolean query(struct query **new_qry, struct db *db, const char *query,
			const char *fmt, ...)
{
	struct query_params *params;
	va_list ap;
	gboolean ret;

	params = query_params_new(SQL_PARAM_INPUT);
	va_start(ap, fmt);
	if (!param_set_group(params, 0, fmt, ap)) {
		db->result = SQLITE_MISUSE;
		sqlerr(db, "Invalid format string");
		query_params_free(params);
		va_end(ap);
		return FALSE;
	}
	va_end(ap);
	ret = query_v(new_qry, db, query, params);
	query_params_free(params);
	return ret;
}

gboolean query_next(struct query *qry)
{
	int result;

	if (g_atomic_int_get(&qry->db->interrupt)) {
		/* Try to stop the query.  If this succeeds, the transaction
		   will be automatically rolled back.  Often, though, the
		   attempt will not succeed. */
		sqlite3_interrupt(qry->db->conn);
	}
	result=sqlite3_step(qry->stmt);
	/* Collapse DONE into OK, since they're semantically equivalent and
	   it simplifies error checking */
	if (result == SQLITE_DONE)
		result=SQLITE_OK;
	/* Collapse IOERR_BLOCKED into BUSY, likewise */
	if (result == SQLITE_IOERR_BLOCKED)
		result=SQLITE_BUSY;
	qry->db->result=result;
	if (result == SQLITE_OK || result == SQLITE_ROW) {
		return TRUE;
	} else {
		sqlerr(qry->db, "%s", sqlite3_errmsg(qry->db->conn));
		return FALSE;
	}
}

gboolean query_has_row(struct db *db)
{
	db_assert_trans(db);
	return (db->result == SQLITE_ROW);
}

gboolean query_ok(struct db *db)
{
	db_assert_trans(db);
	return (db->result == SQLITE_OK);
}

gboolean query_busy(struct db *db)
{
	db_assert_trans(db);
	return (db->result == SQLITE_BUSY);
}

gboolean query_constrained(struct db *db)
{
	db_assert_trans(db);
	return (db->result == SQLITE_CONSTRAINT);
}

void query_row_v(struct query *qry, struct query_params *params)
{
	struct sqlite3_stmt *stmt=qry->stmt;
	struct param *param;
	GList *cur;
	int i;

	if (params->type != SQL_PARAM_OUTPUT) {
		g_critical("Query parameters are not of type output");
		return;
	}
	for (cur = params->list; cur != NULL; cur = cur->next) {
		param = cur->data;
		i = param->index;
		switch (param->type) {
		case 'd':
			*param->o_int = sqlite3_column_int(stmt, i);
			break;
		case 'D':
			*param->o_int64 = sqlite3_column_int64(stmt, i);
			break;
		case 'f':
			*param->o_float = sqlite3_column_double(stmt, i);
			break;
		case 's':
			*param->o_string = sqlite3_column_text(stmt, i);
			break;
		case 'S':
			*param->o_string = sqlite3_column_text(stmt, i);
			*param->o_int = sqlite3_column_bytes(stmt, i);
			break;
		case 'b':
			*param->o_data = sqlite3_column_blob(stmt, i);
			*param->o_int = sqlite3_column_bytes(stmt, i);
			break;
		case 'n':
			*param->o_int = sqlite3_column_bytes(stmt, i);
			break;
		default:
			g_assert_not_reached();
		}
	}
}

void query_row(struct query *qry, const char *fmt, ...)
{
	struct query_params *params;
	va_list ap;

	params = query_params_new(SQL_PARAM_OUTPUT);
	va_start(ap, fmt);
	param_set_group(params, 0, fmt, ap);
	va_end(ap);
	query_row_v(qry, params);
	query_params_free(params);
}

void query_free(struct query *qry)
{
	unsigned ms;

	if (qry == NULL)
		return;

	ms = g_timer_elapsed(qry->timer, NULL) * 1000;
	/* COMMIT is frequently slow, but we don't learn anything by logging
	   that, and it clutters up the logs */
	if (ms >= SLOW_THRESHOLD_MS && strcmp(qry->sql, "COMMIT"))
		g_log(G_LOG_DOMAIN, SQL_LOG_LEVEL_SLOW_QUERY,
					"Slow query took %u ms: \"%s\"",
					ms, qry->sql);
	g_log(G_LOG_DOMAIN, SQL_LOG_LEVEL_QUERY, "Query took %u ms: \"%s\"",
				ms, qry->sql);

	g_timer_destroy(qry->timer);
	sqlite3_finalize(qry->stmt);
	qry->db->queries--;
	g_slice_free(struct query, qry);
}

void sql_log_err(struct db *db, const char *fmt, ...)
{
	gchar *str;
	va_list ap;

	db_assert_trans(db);
	if (db->result != SQLITE_BUSY && db->result != SQLITE_INTERRUPT) {
		va_start(ap, fmt);
		str = g_strdup_vprintf(fmt, ap);
		va_end(ap);
		if (db->result != SQLITE_ROW && db->result != SQLITE_OK)
			g_message("%s (%d, %s)", str, db->result, db->errmsg);
		else
			g_message("%s", str);
		g_free(str);
	}
}

void sql_init(void)
{
	if (!g_thread_supported())
		g_thread_init(NULL);
	srandom(time(NULL));
}

static int busy_handler(void *data, int count)
{
	struct db *db = data;
	long time;

	if (count == 0)
		db->busy_queries++;
	if (count >= 10) {
		db->busy_timeouts++;
		return 0;
	}
	time=random() % (MAX_WAIT_USEC/2);
	db->wait_usecs += time;
	usleep(time);
	return 1;
}

static int progress_handler(void *data)
{
	struct db *db = data;

	return g_atomic_int_get(&db->interrupt);
}

static gboolean sql_setup_db(struct db *db, const char *name)
{
	gchar *str;

	/* SQLite won't let us use a prepared statement parameter for the
	   database name. */
	str = g_strdup_printf("PRAGMA %s.synchronous = OFF", name);
again:
	if (!query(NULL, db, str, NULL)) {
		if (query_busy(db)) {
			query_backoff(db);
			goto again;
		}
		g_free(str);
		sql_log_err(db, "Couldn't set synchronous pragma for "
					"%s database", name);
		return FALSE;
	}
	g_free(str);
	return TRUE;
}

gboolean sql_conn_open(const char *path, struct db **handle)
{
	struct db *db;

	*handle = NULL;
	db = g_slice_new0(struct db);
	pthread_mutex_init(&db->lock, NULL);
	g_atomic_int_set(&db->interrupt, FALSE);
	db_get(db);
	if (sqlite3_open(path, &db->conn)) {
		g_message("Couldn't open database %s: %s", path,
					sqlite3_errmsg(db->conn));
		db_put(db);
		pthread_mutex_destroy(&db->lock);
		g_slice_free(struct db, db);
		return FALSE;
	}
	db->file = g_strdup(path);
	if (sqlite3_extended_result_codes(db->conn, 1)) {
		g_message("Couldn't enable extended result codes for "
					"database %s", path);
		goto bad;
	}
	if (sqlite3_busy_handler(db->conn, busy_handler, db)) {
		g_message("Couldn't set busy handler for database %s", path);
		goto bad;
	}
	/* Every so often during long-running queries, check to see if a
	   signal is pending */
	sqlite3_progress_handler(db->conn, PROGRESS_HANDLER_INTERVAL,
				progress_handler, db);
again:
	if (!query(NULL, db, "PRAGMA count_changes = TRUE", NULL)) {
		if (query_busy(db)) {
			query_backoff(db);
			goto again;
		}
		sql_log_err(db, "Couldn't enable count_changes for %s", path);
		goto bad;
	}
	if (!sql_setup_db(db, "main"))
		goto bad;
	db_put(db);
	*handle = db;
	return TRUE;

bad:
	sqlite3_close(db->conn);
	g_free(db->file);
	db_put(db);
	pthread_mutex_destroy(&db->lock);
	g_slice_free(struct db, db);
	return FALSE;
}

void sql_conn_close(struct db *db)
{
	if (db == NULL)
		return;
	if (sqlite3_close(db->conn))
		g_message("Couldn't close database: %s",
					sqlite3_errmsg(db->conn));
	pthread_mutex_destroy(&db->lock);
	g_free(db->errmsg);
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s: Busy handler called for "
				"%u queries; %u timeouts", db->file,
				db->busy_queries, db->busy_timeouts);
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "%s: %u SQL retries; %llu ms "
				"spent in backoffs", db->file, db->retries,
				(unsigned long long) db->wait_usecs / 1000);
	g_free(db->file);
	g_slice_free(struct db, db);
}

/* This should not be called inside a transaction, since the whole point of
   sleeping is to do it without locks held */
void query_backoff(struct db *db)
{
	long time;

	/* The SQLite busy handler is not called when SQLITE_BUSY results
	   from a failed attempt to promote a shared lock to reserved.  So
	   we can't just retry after getting SQLITE_BUSY; we have to back
	   off first. */
	time=random() % MAX_WAIT_USEC;
	db->wait_usecs += time;
	usleep(time);
	db->retries++;
}

void query_interrupt(struct db *db)
{
	g_atomic_int_set(&db->interrupt, TRUE);
}

void query_clear_interrupt(struct db *db)
{
	g_atomic_int_set(&db->interrupt, FALSE);
}

gboolean attach(struct db *db, const char *handle, const char *file)
{
	gboolean ret = TRUE;

	db_get(db);
again:
	if (!query(NULL, db, "ATTACH ? AS ?", "ss", file, handle)) {
		if (query_busy(db)) {
			query_backoff(db);
			goto again;
		}
		sql_log_err(db, "Couldn't attach %s", file);
		ret = FALSE;
		goto out;
	}
	if (!sql_setup_db(db, handle)) {
		ret = FALSE;
again_detach:
		if (!query(NULL, db, "DETACH ?", "s", handle)) {
			if (query_busy(db)) {
				query_backoff(db);
				goto again_detach;
			}
			sql_log_err(db, "Couldn't detach %s", handle);
		}
	}
out:
	db_put(db);
	return ret;
}

gboolean _begin(struct db *db, gboolean transaction, gboolean immediate)
{
	db_get(db);
	db->use_transaction = transaction;
	if (!transaction)
		return TRUE;
again:
	if (!query(NULL, db, immediate ? "BEGIN IMMEDIATE" : "BEGIN", NULL)) {
		if (query_busy(db))
			goto again;
		sql_log_err(db, "Couldn't begin transaction");
		db_put(db);
		return FALSE;
	}
	return TRUE;
}

gboolean commit(struct db *db)
{
again:
	if (db->use_transaction) {
		if (!query(NULL, db, "COMMIT", NULL)) {
			if (query_busy(db))
				goto again;
			sql_log_err(db, "Couldn't commit transaction");
			return FALSE;
		}
	}
	db_put(db);
	return TRUE;
}

gboolean rollback(struct db *db)
{
	if (!db->use_transaction) {
		g_critical("Can't roll back transactions opened with "
					"begin_bare()");
		return FALSE;
	}
again:
	/* Several SQLite errors *sometimes* result in an automatic rollback.
	   Always try to roll back, just to be safe, but don't report an error
	   if no transaction is active afterward, even if the rollback claimed
	   to fail. */
	if (!query(NULL, db, "ROLLBACK", NULL) &&
				!sqlite3_get_autocommit(db->conn)) {
		if (query_busy(db))
			goto again;
		sql_log_err(db, "Couldn't roll back transaction");
		return FALSE;
	} else {
		db_put(db);
		return TRUE;
	}
}

gchar *sql_head(struct db *db, const char *sql, const char **sql_tail)
{
	const char *tail;
	gchar *ret;
	unsigned len;
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(db->conn, sql, -1, &stmt, &tail)) {
		g_message("Couldn't parse SQL statement: %s",
					sqlite3_errmsg(db->conn));
		if (sql_tail != NULL)
			*sql_tail = sql;
		return NULL;
	}
	sqlite3_finalize(stmt);
	len = tail - sql;
	ret = g_malloc(len + 1);
	ret[len] = 0;
	memcpy(ret, sql, len);
	if (sql_tail != NULL)
		*sql_tail = tail;
	return ret;
}

int query_parameter_count(struct db *db, const char *sql)
{
	sqlite3_stmt *stmt;
	int ret;

	if (sqlite3_prepare_v2(db->conn, sql, -1, &stmt, NULL)) {
		g_message("Couldn't parse SQL statement: %s",
					sqlite3_errmsg(db->conn));
		return -1;
	}
	ret = sqlite3_bind_parameter_count(stmt);
	sqlite3_finalize(stmt);
	return ret;
}

gchar **query_column_names(struct query *qry)
{
	gchar **ret;
	unsigned n;
	unsigned count;

	count = sqlite3_column_count(qry->stmt);
	ret = g_new(gchar *, count + 1);
	ret[count] = NULL;
	for (n = 0; n < count; n++)
		ret[n] = g_strdup(sqlite3_column_name(qry->stmt, n));
	return ret;
}

gchar *query_column_types(struct query *qry)
{
	gchar *ret;
	unsigned n;
	unsigned count;

	count = sqlite3_column_count(qry->stmt);
	ret = g_malloc(count + 1);
	ret[count] = 0;
	for (n = 0; n < count; n++) {
		switch (sqlite3_column_type(qry->stmt, n)) {
		case SQLITE_INTEGER:
			ret[n] = 'd';
			break;
		case SQLITE_FLOAT:
			ret[n] = 'f';
			break;
		case SQLITE_TEXT:
			ret[n] = 's';
			break;
		case SQLITE_BLOB:
			ret[n] = 'b';
			break;
		case SQLITE_NULL:
			ret[n] = '0';
			break;
		default:
			ret[n] = '.';
			break;
		}
	}
	return ret;
}

gboolean vacuum(struct db *db)
{
	gboolean retry;

	db_get(db);
again_vacuum:
	if (!query(NULL, db, "VACUUM", NULL)) {
		sql_log_err(db, "Couldn't vacuum database");
		if (query_busy(db)) {
			query_backoff(db);
			goto again_vacuum;
		} else {
			db_put(db);
			return FALSE;
		}
	}
	db_put(db);

again_trans:
	/* VACUUM flushes the connection's schema cache.  Perform a dummy
	   transaction on the connection to reload the cache; otherwise,
	   the next transaction on the connection would unexpectedly take
	   a lock on all attached databases. */
	if (!begin(db))
		return FALSE;
	if (!query(NULL, db, "SELECT * FROM sqlite_master LIMIT 1", NULL)) {
		sql_log_err(db, "Couldn't query sqlite_master");
		goto bad_trans;
	}
	if (!commit(db))
		goto bad_trans;
	return TRUE;

bad_trans:
	retry = query_busy(db);
	rollback(db);
	if (retry) {
		query_backoff(db);
		goto again_trans;
	}
	return FALSE;
}

/* This validates both the primary and attached databases */
gboolean validate_db(struct db *db)
{
	struct query *qry;
	const char *str;
	int res;

	db_get(db);
again:
	query(&qry, db, "PRAGMA integrity_check(50)", NULL);
	if (query_busy(db)) {
		query_backoff(db);
		goto again;
	} else if (!query_has_row(db)) {
		sql_log_err(db, "Couldn't run SQLite integrity check");
		db_put(db);
		return FALSE;
	}
	query_row(qry, "s", &str);
	res=strcmp(str, "ok");
	if (res) {
		g_message("SQLite integrity check failed");
		for (; query_has_row(db); query_next(qry)) {
			query_row(qry, "s", &str);
			g_message("Integrity: %s", str);
		}
	}
	query_free(qry);
	db_put(db);
	return res ? FALSE : TRUE;
}
