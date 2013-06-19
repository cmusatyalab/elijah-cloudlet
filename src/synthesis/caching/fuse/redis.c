/* cloudletcachefs - cloudlet cachcing emulation fs
 *
 * copyright (c) 2011-2013 carnegie mellon university
 *
 * this program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the gnu general public license as published
 * by the free software foundation.  a copy of the gnu general public license
 * should have been distributed along with this program in the file
 * copying.
 *
 * this program is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  see the gnu general public license
 * for more details.
 */

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#include "cachefs-private.h"

#define REDIS_GET_ATTRIBUTE		"GET %s\u03b1"
#define REDIS_GET_LIST_DIR		"LRANGE %s\u03b2 0 -1"
#define REDIS_KEY_EXISTS		"EXISTS %s\u03b1"
#define REDIS_PUBLISH			"PUBLISH %s %s"


/* Internal structure to encapsulate REDIS connection */
struct redis_handler
{
	// regular redis connection
    redisContext* conn;
    char *redis_ip;
    int redis_port;
    GMutex *lock;

    redisAsyncContext *async;
    struct event_base *event_base;
    pthread_t redis_thread;

};

struct redis_handler* handle = NULL;


/* Internal methods */
static redisContext* redis_connection(struct redis_handler *handle)
{
	redisContext* context = NULL;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	context = redisConnectWithTimeout((char*)handle->redis_ip, handle->redis_port, timeout);
	if (context == NULL || context->err) {
		if (context != NULL) {
			redisFree(context);
		}
		return NULL;
	}
	/* make keepalive connection */
	if (redisEnableKeepAlive(context) != REDIS_OK){
		if (context != NULL) {
			redisFree(context);
		}
		return NULL;
	}
	return context;
}

bool static check_connection(){
	if ((handle != NULL) && (handle->conn != NULL)){
		return true;
	}
	return false;
}

static int check_redis_return(struct redis_handler* handle, redisReply* reply)
{
    if (reply == NULL || ((int64_t) reply) == REDIS_ERR) {
        switch(handle->conn->err) {
            case REDIS_ERR_IO:
                break;
            case REDIS_ERR_EOF:
                break;
            case REDIS_ERR_PROTOCOL:
                break;
            case REDIS_ERR_OTHER:
                break;
            default:
                break;
        };
        return EXIT_FAILURE;
    }
    
    freeReplyObject(reply);
    return EXIT_SUCCESS;
}


/* redis subscribe thread */
static void on_subscribe_msg(redisAsyncContext *c, void *reply, void *privdata) {
	struct cachefs *fs = privdata;
	int i = 0;
    redisReply *r = reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        if (r->elements != 3){
        	return;
		}
		if (g_strcmp0((const char*)r->element[0]->str, "message") != 0){
			//_cachefs_write_debug("received msg is wrong: %s\n", r->element[0]);
        	return;
		}
        const char *request = (char *)r->element[2]->str;
		gchar **fetch_info = g_strsplit(request, ":", 0);
		if ((*fetch_info == NULL) || (*(fetch_info +1) == NULL)){
			_cachefs_write_debug("[redis] Wrong redis message : %s %s %s", \
					r->element[0]->str, r->element[1]->str, r->element[2]->str);
			return;
		}

		gchar *command = g_strdup(*fetch_info);
		gchar *relpath = g_strdup(*(fetch_info+1));
		if (strcmp(command, "fetch") == 0){
			_cachefs_write_debug("[redis] wake up waiting thread for : %s", relpath);
			struct cachefs_cond* cond = g_hash_table_lookup(fs->file_locks, relpath);
			if (cond != NULL){
				_cachefs_cond_broadcast(cond);
				_cachefs_write_debug("[redis] is disallocated: %s", relpath); 
			} else{
				_cachefs_write_debug("[redis] Cannot find any condition for : %s", relpath);
			}
		} else{
			_cachefs_write_error("[redis] Wrong command : %s, %s", command, relpath);
		}
    }
}

static void *redis_subscribe(void *args)
{
	struct cachefs *fs = (struct cachefs *)args;
	redisLibeventAttach(handle->async, handle->event_base);
	redisAsyncCommand(handle->async, on_subscribe_msg, fs, \
			"SUBSCRIBE %s", fs->redis_res_channel);
	event_base_dispatch(handle->event_base);
}

static void *redisCommandAlive(struct redis_handler *handle, const char *format, ...)
{
	GString *new_string = g_string_new("");
    va_list ap;
    void *reply = NULL;
    redisContext *c = handle->conn;
    va_start(ap,format);
    g_string_append_printf(new_string, format, ap);
    reply = redisvCommand(c, format, ap);
    va_end(ap);

    if (reply == NULL || ((int64_t) reply) == REDIS_ERR) {
		/* check connection loss */
		_cachefs_write_debug("[redis] lost connection, reconnect");
        if (handle->conn) {
            redisFree(handle->conn);
        }
		handle->conn = redis_connection(handle);
		if (handle->conn != NULL){
			reply = redisCommand(handle->conn, format, new_string->str);
			_cachefs_write_debug("[redis] retry command : %s, return : %x", \
					new_string->str, reply);
		}else{
			reply = NULL;
		}
    }
    g_string_free(new_string, TRUE);
    return reply;
}


/* public methods */
bool _redis_init(struct cachefs *fs)
{
    redisContext *c;
    redisReply *reply;
    handle = (struct redis_handler*) malloc(sizeof(struct redis_handler));
    handle->redis_ip = g_strdup(fs->redis_ip);
    handle->redis_port = fs->redis_port;
    if (handle) {
		handle->conn = redis_connection(handle);
		if (handle->conn == NULL){
			return false;
		}

		/* set async connection for pub/sub */
		handle->async = redisAsyncConnect(handle->redis_ip, handle->redis_port);
		if (handle->async->err) {
			_cachefs_write_error("redis error in setting async operation : %s\n", c->errstr);
			return false;
		}
		struct event_base *base = event_base_new();
		handle->event_base = base;
		pthread_create(&handle->redis_thread, NULL, redis_subscribe, fs);

		/* PING server */
		reply = redisCommandAlive(handle, "PING");
		if ((reply == NULL) || (strlen(reply->str) <= 0)){
			return false;
		}
		freeReplyObject(reply);
	}

	handle->lock = g_mutex_new();

	return true;
}

void _redis_close()
{
    int outstanding = 0;

    if (handle) {
        if (handle->conn) {
            redisFree(handle->conn);
        }
        if (handle->async) { 
            redisAsyncFree(handle->async);
		}
		if (handle->event_base){
			event_base_loopbreak(handle->event_base);
		}
        if (handle->redis_thread){
        	pthread_cancel(handle->redis_thread);
		}
        g_mutex_free(handle->lock);
        g_free(handle->redis_ip);
        free(handle);
        handle = NULL;
    }
}

struct cachefs_cond* _redis_publish(struct cachefs *fs, char *request_path)
{
    g_mutex_lock(handle->lock);

	// create conditional variable
	struct cachefs_cond* cond = NULL;
	cond = g_hash_table_lookup(fs->file_locks, request_path);
	redisReply* reply = NULL;
	if (cond == NULL){
		cond = _cachefs_cond_new();
		g_hash_table_insert(fs->file_locks, request_path, cond);

		// only the first thread send a request
		const char *request_channel = fs->redis_req_channel;
		_cachefs_write_debug("[redis] publish %s %s", request_channel, request_path);
		reply = redisCommandAlive(handle, REDIS_PUBLISH, request_channel, request_path);
		if ((reply != NULL) && (reply->type == REDIS_REPLY_INTEGER)){
			if (reply->integer == 0){
				_cachefs_write_debug("[redis] No listener for %s\n", request_channel);
			}
		}
	}

    g_mutex_unlock(handle->lock);
    check_redis_return(handle, reply);
    return cond;
}

int _redis_file_exists(const char *path, bool *is_exists)
{
	if (!check_connection())
		return EXIT_FAILURE;

    g_mutex_lock(handle->lock);
    redisReply* reply;
    reply = redisCommandAlive(handle, REDIS_KEY_EXISTS, path);
    if ((reply != NULL) && (reply->type == REDIS_REPLY_INTEGER)){
    	if (reply->integer == 1){
    		*is_exists = true;
    	}else{
    		*is_exists = false;
		}
    }
    g_mutex_unlock(handle->lock);
    return check_redis_return(handle, reply);
}

int _redis_get_attr(const char* path, char** ret_buf)
{ 
	if (!check_connection())
		return EXIT_FAILURE;

    g_mutex_lock(handle->lock);
    redisReply* reply;
    reply = redisCommandAlive(handle, REDIS_GET_ATTRIBUTE, path);
    if ((reply != NULL) && (reply->type == REDIS_REPLY_STRING && reply->len > 0)){
        *ret_buf = g_strndup(reply->str, reply->len);
    } else {
        //_cachefs_write_debug("[redis] cannot get attr from redis %s(%d)\n", path, reply->len);
    }

    g_mutex_unlock(handle->lock);
    return check_redis_return(handle, reply);
}

int _redis_get_readdir(const char* path, GSList **ret_list)
{
	if (!check_connection())
		return EXIT_FAILURE;

    g_mutex_lock(handle->lock);
    redisReply* reply;
	int i = 0;
	gchar *attr_str;
    reply = redisCommandAlive(handle, REDIS_GET_LIST_DIR, path);
    _cachefs_write_debug("[redis] get dir at : %s", path);
    if ((reply != NULL) && (reply->type == REDIS_REPLY_ARRAY)){
        for (i = 0; i < reply->elements; i++) {
        	attr_str = g_strndup(reply->element[i]->str, reply->element[i]->len);
        	*ret_list = g_slist_append(*ret_list, attr_str);
        }
    }else{
    	_cachefs_write_error("[redis] no return for dir : %s", path);
	}
    g_mutex_unlock(handle->lock);
    return check_redis_return(handle, reply);
}
