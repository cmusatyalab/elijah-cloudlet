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
#include <unistd.h>
#include <curl/curl.h>
#include "vmnetfs-private.h"

#define TRANSPORT_TRIES 5
#define TRANSPORT_RETRY_DELAY 5

struct connection_pool {
    GQueue *conns;
    GMutex *lock;
};

struct connection {
    struct connection_pool *pool;
    CURL *curl;
    char errbuf[CURL_ERROR_SIZE];
    char *buf;
    uint64_t offset;
    uint64_t length;
};

static size_t write_callback(void *data, size_t size, size_t nmemb,
        void *private)
{
    struct connection *conn = private;
    uint64_t count = MIN(size * nmemb, conn->length - conn->offset);

    memcpy(conn->buf + conn->offset, data, count);
    conn->offset += count;
    return count;
}

static int progress_callback(void *private G_GNUC_UNUSED,
        double dltotal G_GNUC_UNUSED, double dlnow G_GNUC_UNUSED,
        double ultotal G_GNUC_UNUSED, double ulnow G_GNUC_UNUSED)
{
    return _vmnetfs_interrupted();
}

static void conn_free(struct connection *conn)
{
    if (conn->curl) {
        curl_easy_cleanup(conn->curl);
    }
    g_slice_free(struct connection, conn);
}

static struct connection *conn_new(struct connection_pool *pool,
        GError **err)
{
    struct connection *conn;

    conn = g_slice_new0(struct connection);
    conn->pool = pool;
    conn->curl = curl_easy_init();
    if (conn->curl == NULL) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't initialize CURL handle");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_NOPROGRESS, 0)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't enable curl progress meter");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_NOSIGNAL, 1)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't disable signals");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_HTTPAUTH,
            CURLAUTH_BASIC | CURLAUTH_DIGEST)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't configure authentication");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_WRITEFUNCTION, write_callback)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set write callback");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_WRITEDATA, conn)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set write callback data");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_PROGRESSFUNCTION,
            progress_callback)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set progress callback");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_ERRORBUFFER, conn->errbuf)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set error buffer");
        goto bad;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_FAILONERROR, 1)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set fail-on-error flag");
        goto bad;
    }
    return conn;

bad:
    conn_free(conn);
    return NULL;
}

static struct connection *conn_get(struct connection_pool *cpool,
        GError **err)
{
    struct connection *conn;

    g_mutex_lock(cpool->lock);
    conn = g_queue_pop_head(cpool->conns);
    g_mutex_unlock(cpool->lock);
    if (conn == NULL) {
        conn = conn_new(cpool, err);
    }
    return conn;
}

static void conn_put(struct connection *conn)
{
    g_mutex_lock(conn->pool->lock);
    g_queue_push_head(conn->pool->conns, conn);
    g_mutex_unlock(conn->pool->lock);
}

bool _vmnetfs_transport_init(void)
{
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        return false;
    }
    return true;
}

struct connection_pool *_vmnetfs_transport_pool_new(void)
{
    struct connection_pool *cpool;

    cpool = g_slice_new0(struct connection_pool);
    cpool->conns = g_queue_new();
    cpool->lock = g_mutex_new();
    return cpool;
}

void _vmnetfs_transport_pool_free(struct connection_pool *cpool)
{
    struct connection *conn;

    while ((conn = g_queue_pop_head(cpool->conns)) != NULL) {
        conn_free(conn);
    }
    g_queue_free(cpool->conns);
    g_mutex_free(cpool->lock);
    g_slice_free(struct connection_pool, cpool);
}

/* Make one attempt to fetch the specified byte range from the URL. */
static bool fetch(struct connection_pool *cpool, const char *url,
        const char *username, const char *password, void *buf,
        uint64_t offset, uint64_t length, GError **err)
{
    struct connection *conn;
    char *range;
    bool ret = false;
    CURLcode code;

    conn = conn_get(cpool, err);
    if (conn == NULL) {
        return false;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_URL, url)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set connection URL");
        goto out;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_USERNAME, username)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set authentication username");
        goto out;
    }
    if (curl_easy_setopt(conn->curl, CURLOPT_PASSWORD, password)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set authentication password");
        goto out;
    }
    range = g_strdup_printf("%"PRIu64"-%"PRIu64, offset, offset + length - 1);
    if (curl_easy_setopt(conn->curl, CURLOPT_RANGE, range)) {
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "Couldn't set transfer byte range");
        g_free(range);
        goto out;
    }
    g_free(range);
    conn->buf = buf;
    conn->offset = 0;
    conn->length = length;

    code = curl_easy_perform(conn->curl);
    switch (code) {
    case CURLE_OK:
        if (conn->offset != length) {
            g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                    VMNETFS_TRANSPORT_ERROR_FATAL,
                    "short read from server: %"PRIu64"/%"PRIu64,
                    conn->offset, length);
        } else {
            ret = true;
        }
        break;
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
    case CURLE_HTTP_RETURNED_ERROR:
    case CURLE_OPERATION_TIMEDOUT:
    case CURLE_GOT_NOTHING:
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_BAD_CONTENT_ENCODING:
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_NETWORK,
                "curl error %d: %s", code, conn->errbuf);
        break;
    case CURLE_ABORTED_BY_CALLBACK:
        g_set_error(err, VMNETFS_IO_ERROR, VMNETFS_IO_ERROR_INTERRUPTED,
                "Operation interrupted");
        break;
    default:
        g_set_error(err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_FATAL,
                "curl error %d: %s", code, conn->errbuf);
        break;
    }
out:
    conn_put(conn);
    return ret;
}

/* Attempt to fetch the specified byte range from the URL, retrying
   several times in case of retryable errors. */
bool _vmnetfs_transport_fetch(struct connection_pool *cpool, const char *url,
        const char *username, const char *password, void *buf,
        uint64_t offset, uint64_t length, GError **err)
{
    GError *my_err = NULL;
    int i;

    for (i = 0; i < TRANSPORT_TRIES; i++) {
        if (my_err != NULL) {
            g_clear_error(&my_err);
            sleep(TRANSPORT_RETRY_DELAY);
        }
        if (fetch(cpool, url, username, password, buf, offset, length,
                &my_err)) {
            return true;
        }
        if (!g_error_matches(my_err, VMNETFS_TRANSPORT_ERROR,
                VMNETFS_TRANSPORT_ERROR_NETWORK)) {
            /* fatal error */
            break;
        }
    }
    g_propagate_error(err, my_err);
    return false;
}
