//
// Created by haoba on 17-7-23.
//

#ifndef TKDATABASE_ZK_ADAPTOR_H
#define TKDATABASE_ZK_ADAPTOR_H

#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "proto.h"
#include "zookeeperServer.recordio.h"


/* the size of connect request */
#define HANDSHAKE_REQ_SIZE 44
/* connect request */
struct connect_req {
    int32_t protocolVersion;
    int64_t lastZxidSeen;
    int32_t timeOut;
    int64_t sessionId;
    int32_t passwd_len;
    char passwd[16];
};

/* the connect response */
struct prime_struct {
    int32_t len;
    int32_t protocolVersion;
    int32_t timeOut;
    int64_t sessionId;
    int32_t passwd_len;
    char passwd[16];
};

/**
 * This structure represents a packet being read or written.
 */
typedef struct _buffer_list {
    char *buffer;
    int32_t len; /* This represents the length of sizeof(header) + length of buffer */
    int64_t sessionId;
    int32_t ownerOrfd;
    struct _buffer_list *next;
} buffer_list_t;

typedef struct _buffer_head {
    int32_t bufferLength;
    struct _buffer_list *volatile head;
    struct _buffer_list *last;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} buffer_head_t;


/*typedef struct completion {
    int type; /* one of COMPLETION_* values above */
/*    union {
        void_completion_t void_result;
        stat_completion_t stat_result;
        data_completion_t data_result;
        strings_completion_t strings_result;
        strings_stat_completion_t strings_stat_result;
        acl_completion_t acl_result;
        string_completion_t string_result;
        struct watcher_object_list *watcher_result;
    };
} completion_t;

typedef struct _completion_list {
    int xid;
    completion_t c;
    const void *data;
    buffer_list_t *buffer;
    struct _completion_list *next;
    watcher_registration_t* watcher;
} completion_list_t;*/




int lock_buffer_list(buffer_head_t *l);
int unlock_buffer_list(buffer_head_t *l);

buffer_list_t *allocate_buffer(char *buff, int32_t len,int64_t sessionId);
buffer_list_t *dequeue_buffer(buffer_head_t *list);
int remove_buffer(buffer_head_t *list);
void queue_buffer(buffer_head_t *list, buffer_list_t *b, int add_to_front);
int queue_buffer_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId,int32_t fd);
int queue_buffer_reply_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId,int32_t fd);
int queue_front_buffer_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId);
int get_queue_len(buffer_head_t *list);


#endif //TKDATABASE_ZK_ADAPTOR_H
