/*
 * cloudletcachefs - cloudlet cachcing emulation fs
 *
 * copyright (c) 2006-2012 carnegie mellon university
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
#include "hiredis.h"
#include "cachefs-private.h"

bool _redis_init(const char *address, int port)
{
    redisContext *c;
    redisReply *reply;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout((char*)address, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            redisFree(c);
		}
        return false;
    }

    /* PING server */
    reply = redisCommand(c,"PING");
    if ((reply == NULL) || (strlen(reply->str) <= 0)){
    	return false;
	}
	freeReplyObject(reply);

	return true;
}

void _redis_close()
{
}
