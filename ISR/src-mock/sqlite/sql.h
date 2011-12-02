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

#ifndef ISR_SQL_H
#define ISR_SQL_H

#include <glib.h>

/* glib log domain:
 *	isrsql
 * Log levels:
 *	G_LOG_LEVEL_CRITICAL		- Programmer errors
 *	G_LOG_LEVEL_MESSAGE		- Ordinary errors
 *	G_LOG_LEVEL_INFO		- Statistics
 *	SQL_LOG_LEVEL_QUERY		- Query strings
 *	SQL_LOG_LEVEL_SLOW_QUERY	- Slow-query warnings
 */
enum sql_log_level {
	SQL_LOG_LEVEL_QUERY		= 1 << G_LOG_LEVEL_USER_SHIFT,
	SQL_LOG_LEVEL_SLOW_QUERY	= 1 << (G_LOG_LEVEL_USER_SHIFT + 1),
};

enum sql_param_type {
	SQL_PARAM_INPUT			= 0,
	SQL_PARAM_OUTPUT		= 1,
};

struct db;
struct query;
struct query_params;


/***** Basic functions *****/

/* Initialize the library. */
void sql_init(void);

/* Open and return a database connection to the given database file, which
   need not exist.  Returns FALSE on error. */
gboolean sql_conn_open(const char *path, struct db **handle);

/* Close the given database connection. */
void sql_conn_close(struct db *db);

/* Start the SQL query @query against the given @db.  @fmt is a string
   containing individual characters representing the types of the positional
   parameters:
	d - int
	D - int64_t
	f - double
	s - string (to be copied into the query structure)
	S - string (not copied; must be constant until *new_qry is freed)
	b - blob (copied into the query) - void * (data) + int (length)
	B - blob (not copied) - void * (data) + int (length)
   @fmt may be NULL if there are no positional parameters.  @new_qry may be
   NULL, even if the query is a SELECT.  This will set the status flags
   queried by query_has_row(), etc., without returning a query structure.
   If @new_qry is non-NULL, *new_qry will be set to a query structure if
   at least one row is being returned, and to NULL otherwise.  This function
   returns TRUE if query_ok() or query_has_row() would return TRUE, and
   FALSE otherwise. */
gboolean query(struct query **new_qry, struct db *db, const char *query,
			const char *fmt, ...);

/* Step to the next row of results.  Returns TRUE if query_ok() or
   query_has_row() would return TRUE, and FALSE otherwise. */
gboolean query_next(struct query *qry);

/* Returns TRUE if the last database operation caused a row to be returned,
   FALSE otherwise.  Note that INSERT/UPDATE/DELETE statements return a row
   with a single column giving the number of rows that were modified. */
gboolean query_has_row(struct db *db);

/* Returns TRUE if the last database operation succeeded WITHOUT returning a
   row, FALSE otherwise.  Note that INSERT/UPDATE/DELETE statements return
   a row with a single column giving the number of rows that were modified. */
gboolean query_ok(struct db *db);

/* Returns TRUE if the last database operation failed because of contention
   for the database file (such that rolling back and retrying the transaction
   might succeed), FALSE otherwise. */
gboolean query_busy(struct db *db);

/* Returns TRUE if the last database operation failed due to a constraint
   violation, FALSE otherwise. */
gboolean query_constrained(struct db *db);

/* Fetch the current row of data from the query.  @fmt is a string containing
   individual characters representing the data types of the positional
   parameters, which are pointers to values to be filled in with the contents
   of each column in turn.  The column contents will be coerced to the
   requested type if possible.  The available types are:
	d - int *
	D - int64_t *
	f - double *
	s - const unsigned char **
	S - const unsigned char ** (string) + int * (length)
	b - const void ** (data) + int * (length)
	n - int * (blob length)
   Returned pointer values are valid until the next row is accessed. */
void query_row(struct query *qry, const char *fmt, ...);

/* Free an allocated query.  If @qry is NULL, does nothing.  All queries must
   be freed before ending a transaction. */
void query_free(struct query *qry);

/* Sleeps for a random interval in order to do backoff on @db after
   query_busy() returns TRUE.  A typical transaction looks like this:

	gboolean retry;

again:
	if (begin(db))
		return FALSE;
	if (!query(...)) {
		sql_log_err(db, "Couldn't frob the database");
		goto bad;
	}
	...
	if (commit(db))
		goto bad;
	return TRUE;

bad:
	retry = query_busy(db);
	rollback(db);
	if (retry) {
		query_backoff(db);
		goto again;
	}
	return FALSE;
  */
void query_backoff(struct db *db);

/* Set the interrupt flag on @db.  When the flag is set, query operations will
   eventually fail (but may not fail immediately or every time).  This
   function is signal-handler- and thread-safe. */
void query_interrupt(struct db *db);

/* Clear the interrupt flag on @db.  This function is signal-handler- and
   thread-safe. */
void query_clear_interrupt(struct db *db);

/* Log the most recent SQL error on @db, including the SQLite error code and
   error detail string.  The message will be logged at level
   G_LOG_LEVEL_MESSAGE.  No message will be logged if the error is retryable
   (i.e. query_busy() would return TRUE) or if it occurred as a result of
   query_interrupt(); this is a feature intended to prevent spurious log
   messages in correctible failure cases. */
void sql_log_err(struct db *db, const char *fmt, ...);

/* Attach an additional database @file (which need not exist) to the @db
   handle, giving it the shortname @handle.  Return TRUE on success, FALSE
   otherwise.  This function performs query_busy() and query_backoff()
   internally. */
gboolean attach(struct db *db, const char *handle, const char *file);

/* Begin a transaction against @db.  All queries must be done in the context
   of a transaction opened in the same thread.  If another thread already has
   a transaction open, _begin() will block until it is committed or rolled
   back.  Returns FALSE on error.  This function performs query_busy() and
   query_backoff() internally. */
gboolean _begin(struct db *db, gboolean transaction, gboolean immediate);
#define begin(db) _begin(db, TRUE, FALSE)
#define begin_immediate(db) _begin(db, TRUE, TRUE)
#define begin_bare(db) _begin(db, FALSE, FALSE)

/* Commit the open transaction against @db.  All queries must have been freed.
   Returns FALSE on error.  This function performs query_busy() and
   query_backoff() internally. */
gboolean commit(struct db *db);

/* Roll back the open transaction on @db.  All queries must have been freed.
   Returns FALSE on error.  This function performs query_busy() and
   query_backoff() internally. */
gboolean rollback(struct db *db);


/***** Dynamic query parameters *****/

/* Allocate a query_params struct.  @type indicates whether the struct will
   be used for input parameters (i.e., for query_params()) or output parameters
   (query_row_params()). */
struct query_params *query_params_new(enum sql_param_type type);

/* Free a query_params struct. */
void query_params_free(struct query_params *params);

/* Add a parameter to @params, corresponding to the given parameter @index
   in the query string.  @type is one of the type characters accepted by
   query() (for input parameters) or query_row() (for output parameters).
   This function never copies the contents of non-primitive types; if
   such copying is requested, it is done when @params is passed to
   query_v().  Returns FALSE on error. */
gboolean query_param_set(struct query_params *params, int index, char type,
			...);

/* Equivalent to query(), but with a dynamically-constructed parameter list. */
gboolean query_v(struct query **new_qry, struct db *db, const char *query,
			struct query_params *params);

/* Equivalent to query_row(), but with a dynamically-constructed parameter
   list. */
void query_row_v(struct query *qry, struct query_params *params);

/* Return an array of names of columns in the result set of the specified
   @qry.  The returned array should be freed with g_strfreev(). */
gchar **query_column_names(struct query *qry);

/* Return a string containing one type character for each column in the current
   row of results for the specified @qry.  The returned string should be freed
   with g_free().  This function must be called *before* obtaining any data
   from the current row using query_row()/query_row_v().  The type character
   will be one of:
   	d - Integer
	f - Floating-point
	s - String
	b - Blob
	0 - Null
	. - Unknown
 */
gchar *query_column_types(struct query *qry);

/***** Utility functions *****/

/* Return a copy of the first SQL statement in @sql.  If @sql_tail is non-NULL,
   return a pointer to the remainder of @sql in *sql_tail.  Must be performed
   inside a transaction.  The returned string should be freed with g_free().
   If the first statement in @sql fails to parse, returns NULL. */
gchar *sql_head(struct db *db, const char *sql, const char **sql_tail);

/* Return the number of positional parameters in the given SQL statement.
   Must be performed inside a transaction.  If @sql fails to parse, returns
   -1.  If @sql is a compound statement, only the first statement is
   considered. */
int query_parameter_count(struct db *db, const char *sql);

/* Reorganize the tables of @db for faster access.  Must be performed outside
   a transaction.  Returns FALSE on error.  This function performs
   query_busy() and query_backoff() internally. */
gboolean vacuum(struct db *db);

/* Perform an internal consistency check on the main and attached databases
   of @db.  Must be performed outside a transaction.  Returns FALSE if the
   consistency check fails, or on other error.  This function performs
   query_busy() and query_backoff() internally. */
gboolean validate_db(struct db *db);

#endif
