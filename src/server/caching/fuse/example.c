#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
	int j = 0;
    redisReply *r = reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }
}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        printf("error: %s\n", c->errstr);
        return 1;
    }

    redisLibeventAttach(c, base);
    redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE foo");
    event_base_dispatch(base);
    return 0;
}
