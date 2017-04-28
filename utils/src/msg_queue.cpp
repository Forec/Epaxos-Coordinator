//
// Created by forec on 17-4-27.
//

#include "../include/msg_queue.h"
#include <stdio.h>

MsgQueue_t * initMsgQueue(unsigned long _cap, unsigned int _msgSize) {
    MsgQueue_t * mq = (MsgQueue_t *)malloc(sizeof(MsgQueue_t));
    mq->used = 0;
    mq->cap = _cap * _msgSize;
    mq->cnt = _cap;
    mq->size = 0;
    mq->msgSize = _msgSize;
    mq->buf = (char *)malloc(mq->cap);
    pthread_mutex_init(&mq->mutex, NULL);
    sem_init(&mq->empty, mq->cnt, mq->cnt);
    sem_init(&mq->full, mq->cnt, 0);
    return mq;
}

void destroyMsgQueue(MsgQueue_t * mq) {
    pthread_mutex_destroy(&mq->mutex);
    sem_destroy(&mq->empty);
    sem_destroy(&mq->full);
    free(mq->buf);
    free(mq);
}

long availableMsgCount(MsgQueue_t * mq) {
    if (!mq)
        return -1;
    int pending;
    sem_getvalue(&mq->full, &pending);
    return pending;
}

int hasNextMsg(MsgQueue_t * mq) {
    if (!mq) {
        return -1;
    }
    return availableMsgCount(mq) > 0;
}

void* getNextMsg(MsgQueue_t *mq) {
    if (!mq)
        return NULL;
    void * p;
    sem_wait(&mq->full);
    pthread_mutex_lock(&mq->mutex);
    p = (void *)(mq->buf + mq->used);
    mq->used += mq->msgSize;
    if (mq->used >= mq->cap) {
        mq->used = 0;
    }
    pthread_mutex_unlock(&mq->mutex);
    sem_post(&mq->empty);
    return p;
}

void putIntoMsgQueue(MsgQueue_t * mq, void * src) {
    if (!mq)
        return;
    sem_wait(&mq->empty);
    pthread_mutex_lock(&mq->mutex);
    if (mq->size == mq->cap) {
        mq->size = 0;
    }
    unsigned long pos = mq->size;
    mq->size += mq->msgSize;
    pthread_mutex_unlock(&mq->mutex);
    memcpy(mq->buf + pos, src, mq->msgSize);
    sem_post(&mq->full);
}
