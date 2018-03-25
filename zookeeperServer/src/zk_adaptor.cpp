//
// Created by haoba on 17-7-28.
//

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include "../include/zk_adaptor.h"

int lock_buffer_list(buffer_head_t *l)
{
    return pthread_mutex_lock(&l->lock);
}
int  unlock_buffer_list(buffer_head_t *l)
{
    return pthread_mutex_unlock(&l->lock);
}

buffer_list_t *allocate_buffer(char *buff, int32_t len,int64_t sessionId)
{
    buffer_list_t *buffer = (buffer_list_t*)calloc(1, sizeof(*buffer));
    if (buffer == nullptr)
        return nullptr;
    buffer->len = len==0?sizeof(*buffer):len;
    buffer->sessionId = sessionId;
    buffer->buffer = (char *)malloc(len);
    memcpy(buffer->buffer,buff,len);
    buffer->next = nullptr;
    return buffer;
}

void free_buffer(buffer_list_t *b)
{
    if (!b) {
        return;
    }
    if (b->buffer) {
        free(b->buffer);
    }
    free(b);
}

buffer_list_t * dequeue_buffer(buffer_head_t *list)
{
    buffer_list_t * b;
    lock_buffer_list(list);
    if(list->bufferLength == 0){
        pthread_cond_wait(&list->cond,&list->lock);
    }
    b = list->head;
    if (b) {
        list->head = b->next;
        if (!list->head) {
            assert(b == list->last);
            list->last = nullptr;
        }
    }
    list->bufferLength--;
    unlock_buffer_list(list);
    return b;
}

int remove_buffer(buffer_head_t *list)
{
    buffer_list_t *b;
    b = dequeue_buffer(list);
    if (!b) {
        return 0;
    }
    free_buffer(b);
    return 1;
}

void queue_buffer(buffer_head_t *list, buffer_list_t *b, int add_to_front)
{
    b->next = nullptr;
    lock_buffer_list(list);
    if (list->head) {
        assert(list->last);
        // The list is not empty
        if (add_to_front) {
            b->next = list->head;
            list->head = b;
        } else {
            list->last->next = b;
            list->last = b;
        }
    }else{
        // The list is empty
        list->bufferLength = 0;
        assert(!list->head);
        list->head = b;
        list->last = b;
    }
    list->bufferLength++;
    pthread_cond_signal(&list->cond);
    unlock_buffer_list(list);
}

int queue_buffer_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId,int32_t fd)
{
    buffer_list_t *b  = allocate_buffer(buff,len,sessionId);
    b->ownerOrfd = fd;
    if (!b)
        return ZSYSTEMERROR;
    queue_buffer(list, b, 0);
    return ZOK;
}

int queue_buffer_reply_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId,int32_t fd)
{
    int32_t rlen = htonl(len - sizeof(int32_t));
    memcpy(buff,&rlen,sizeof(int32_t));
    int err = queue_buffer_bytes(list,buff,len,sessionId,fd);
    return err;
}

int queue_front_buffer_bytes(buffer_head_t *list, char *buff, int32_t len,int64_t sessionId)
{
    buffer_list_t *b  = allocate_buffer(buff,len,sessionId);
    if (!b)
        return ZSYSTEMERROR;
    queue_buffer(list, b, 1);
    return ZOK;
}

int get_queue_len(buffer_head_t *list)
{
    int i;
    buffer_list_t *ptr;
    lock_buffer_list(list);
    ptr = list->head;
    for (i=0; ptr!=0; ptr=ptr->next, i++)
        ;
    unlock_buffer_list(list);
    return i;
}