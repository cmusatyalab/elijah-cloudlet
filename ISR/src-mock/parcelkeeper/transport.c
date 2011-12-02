/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include "defs.h"

#define TRANSPORT_TRIES 5
#define TRANSPORT_RETRY_DELAY 5

struct pk_connection_pool {
	struct pk_state *state;
	GList *conns;
	GMutex *lock;
};

struct pk_connection {
	struct pk_connection_pool *pool;
	CURL *curl;
	char errbuf[CURL_ERROR_SIZE];
	char *buf;
	size_t offset;
};

static size_t curl_callback(void *data, size_t size, size_t nmemb,
			void *private)
{
	struct pk_connection *conn=private;
	size_t count = MIN(size * nmemb, conn->pool->state->parcel->chunksize -
				conn->offset);

	memcpy(conn->buf + conn->offset, data, count);
	conn->offset += count;
	return count;
}

static void transport_conn_free(struct pk_connection *conn)
{
	if (conn->curl)
		curl_easy_cleanup(conn->curl);
	g_slice_free(struct pk_connection, conn);
}

static struct pk_connection *transport_conn_alloc(
			struct pk_connection_pool *pool)
{
	struct pk_connection *conn;

	conn=g_slice_new0(struct pk_connection);
	conn->pool=pool;
	conn->curl=curl_easy_init();
	if (conn->curl == NULL) {
		pk_log(LOG_ERROR, "Couldn't initialize CURL handle");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_NOPROGRESS, 1)) {
		pk_log(LOG_ERROR, "Couldn't disable curl progress meter");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_NOSIGNAL, 1)) {
		pk_log(LOG_ERROR, "Couldn't disable signals");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION,
				curl_callback)) {
		pk_log(LOG_ERROR, "Couldn't set write callback");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, conn)) {
		pk_log(LOG_ERROR, "Couldn't set write callback data");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_ERRORBUFFER, conn->errbuf)) {
		pk_log(LOG_ERROR, "Couldn't set error buffer");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_FAILONERROR, 1)) {
		pk_log(LOG_ERROR, "Couldn't set fail-on-error flag");
		goto bad;
	}
	if (curl_easy_setopt(conn->curl, CURLOPT_MAXFILESIZE,
				pool->state->parcel->chunksize)) {
		pk_log(LOG_ERROR, "Couldn't set maximum transfer size");
		goto bad;
	}
	return conn;

bad:
	transport_conn_free(conn);
	return NULL;
}

static struct pk_connection *transport_conn_get(
			struct pk_connection_pool *cpool)
{
	struct pk_connection *conn;
	GList *el;

	g_mutex_lock(cpool->lock);
	el = g_list_first(cpool->conns);
	if (el != NULL) {
		conn = el->data;
		cpool->conns = g_list_delete_link(cpool->conns, el);
		g_mutex_unlock(cpool->lock);
		return conn;
	} else {
		g_mutex_unlock(cpool->lock);
		return transport_conn_alloc(cpool);
	}
}

static void transport_conn_put(struct pk_connection *conn)
{
	struct pk_connection_pool *cpool = conn->pool;

	g_mutex_lock(cpool->lock);
	cpool->conns = g_list_prepend(conn->pool->conns, conn);
	g_mutex_unlock(cpool->lock);
}

pk_err_t transport_init(void)
{
	if (curl_global_init(CURL_GLOBAL_ALL)) {
		pk_log(LOG_ERROR, "Couldn't initialize curl library");
		return PK_CALLFAIL;
	}
	return PK_SUCCESS;
}

struct pk_connection_pool *transport_pool_alloc(struct pk_state *state)
{
	struct pk_connection_pool *cpool;

	cpool = g_slice_new0(struct pk_connection_pool);
	cpool->state = state;
	cpool->lock = g_mutex_new();
	return cpool;
}

void transport_pool_free(struct pk_connection_pool *cpool)
{
	GList *el;

	for (el = g_list_first(cpool->conns); el != NULL;
				el = g_list_next(el))
		transport_conn_free(el->data);
	g_list_free(cpool->conns);
	g_mutex_free(cpool->lock);
	g_slice_free(struct pk_connection_pool, cpool);
}

static pk_err_t transport_get(struct pk_connection_pool *cpool, void *buf,
			unsigned chunk, size_t *len)
{
	struct pk_connection *conn;
	gchar *url;
	pk_err_t ret;
	CURLcode err;

	conn = transport_conn_get(cpool);
	if (conn == NULL)
		return PK_CALLFAIL;
	url=form_chunk_path(cpool->state->parcel, cpool->state->parcel->master,
				chunk);
	pk_log(LOG_TRANSPORT, "Fetching %s", url);
	if (curl_easy_setopt(conn->curl, CURLOPT_URL, url)) {
		pk_log(LOG_ERROR, "Couldn't set connection URL");
		g_free(url);
		transport_conn_put(conn);
		return PK_CALLFAIL;
	}
	conn->buf=buf;
	conn->offset=0;
	err=curl_easy_perform(conn->curl);
	if (err)
		pk_log(LOG_ERROR, "Fetching %s: %s", url, conn->errbuf);
	switch (err) {
	case CURLE_OK:
		*len=conn->offset;
		ret=PK_SUCCESS;
		break;
	case CURLE_COULDNT_RESOLVE_PROXY:
	case CURLE_COULDNT_RESOLVE_HOST:
	case CURLE_COULDNT_CONNECT:
	case CURLE_HTTP_RETURNED_ERROR:
	case CURLE_OPERATION_TIMEOUTED:
	case CURLE_GOT_NOTHING:
	case CURLE_SEND_ERROR:
	case CURLE_RECV_ERROR:
	case CURLE_BAD_CONTENT_ENCODING:
		ret=PK_NETFAIL;
		break;
	default:
		ret=PK_IOERR;
		break;
	}
	g_free(url);
	transport_conn_put(conn);
	return ret;
}

pk_err_t transport_fetch_chunk(struct pk_connection_pool *cpool, void *buf,
			unsigned chunk, const void *tag, unsigned *length)
{
	char calctag[cpool->state->parcel->hashlen];
	size_t len=0;  /* Make compiler happy */
	int i;
	pk_err_t ret;

	for (i=0; i<TRANSPORT_TRIES; i++) {
		ret=transport_get(cpool, buf, chunk, &len);
		if (ret != PK_NETFAIL)
			break;
		pk_log(LOG_ERROR, "Fetching chunk %u failed; retrying in %d "
					"seconds", chunk,
					TRANSPORT_RETRY_DELAY);
		sleep(TRANSPORT_RETRY_DELAY);
	}
	if (ret != PK_SUCCESS) {
		pk_log(LOG_ERROR, "Couldn't fetch chunk %u", chunk);
		return ret;
	}
	if (!iu_chunk_crypto_digest(cpool->state->parcel->crypto, calctag,
				buf, len))
		return PK_CALLFAIL;
	if (memcmp(tag, calctag, cpool->state->parcel->hashlen)) {
		pk_log(LOG_ERROR, "Invalid tag for retrieved chunk %u", chunk);
		log_tag_mismatch(tag, calctag, cpool->state->parcel->hashlen);
		return PK_TAGFAIL;
	}
	hoard_put_chunk(cpool->state, tag, buf, len);
	*length=len;
	return PK_SUCCESS;
}
