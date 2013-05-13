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


/* Internal structure to encapsulate REDIS connection */
struct redis_handler
{
    redisContext* conn;
    GMutex *lock;

    redisAsyncContext *async;
    struct event_base *event_base;
    pthread_t redis_thread;

};

struct redis_handler* handle = NULL;


/* Internal methods */
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
			_cachefs_write_debug("[main] Wrong redis message : %s %s %s", \
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
	redisAsyncCommand(handle->async, on_subscribe_msg, fs, "SUBSCRIBE foo");
	event_base_dispatch(handle->event_base);
}

/* public methods */
bool _redis_init(struct cachefs *fs)
{
    const char *address = fs->redis_ip;
    const int port = fs->redis_port;
    const char *redis_sub_channel = fs->redis_res_channel;

    redisContext *c;
    redisReply *reply;
    handle = (struct redis_handler*) malloc(sizeof(struct redis_handler));
    if (handle) {
		struct timeval timeout = { 1, 500000 }; // 1.5 seconds
		handle->conn = redisConnectWithTimeout((char*)address, port, timeout);
		if (handle->conn == NULL || handle->conn->err) {
			if (handle->conn) {
				redisFree(handle->conn);
			}
            free(handle);
			return false;
		}

		/* set async connection for pub/sub */
		handle->async = redisAsyncConnect(address, port);
		if (handle->async->err) {
			_cachefs_write_error("redis error in setting async operation : %s\n", c->errstr);
			return false;
		}
		struct event_base *base = event_base_new();
		handle->event_base = base;
		pthread_create(&handle->redis_thread, NULL, redis_subscribe, fs);

		/* PING server */
		reply = redisCommand(handle->conn, "PING");
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
        free(handle);
        handle = NULL;
    }
}

int _redis_file_exists(const char *path, bool *is_exists)
{
	if (!check_connection())
		return EXIT_FAILURE;

    g_mutex_lock(handle->lock);
    redisReply* reply;
    reply = redisCommand(handle->conn, REDIS_KEY_EXISTS, path);
    if (reply->type == REDIS_REPLY_INTEGER){
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
    reply = redisCommand(handle->conn, REDIS_GET_ATTRIBUTE, path);
    //_cachefs_write_debug(REDIS_GET_ATTRIBUTE, path);
    if (reply->type == REDIS_REPLY_STRING && reply->len > 0){
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
    reply = redisCommand(handle->conn, REDIS_GET_LIST_DIR, path);
    _cachefs_write_debug(REDIS_GET_LIST_DIR, path);
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (i = 0; i < reply->elements; i++) {
        	attr_str = g_strndup(reply->element[i]->str, reply->element[i]->len);
        	*ret_list = g_slist_append(*ret_list, attr_str);
        }
    }else{
    	//fprintf(stderr, "no return\n");
	}
    g_mutex_unlock(handle->lock);
    return check_redis_return(handle, reply);
}
