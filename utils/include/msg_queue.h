//
// Created by forec on 17-4-27.
//

#ifndef __MSG_QUEUE_H_
#define __MSG_QUEUE_H_

#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <semaphore.h>

struct MsgQueue {
    unsigned long used;
    unsigned long size;
    unsigned long cap;
    unsigned long cnt;
    unsigned int msgSize;
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
    char * buf;
};

typedef struct MsgQueue MsgQueue_t;

MsgQueue_t * initMsgQueue(unsigned long _cap, unsigned int _msgSize);

void destroyMsgQueue(MsgQueue_t * mq);

long availableMsgCount(MsgQueue_t * mq);

int hasNextMsg(MsgQueue_t * mq);

void* getNextMsg(MsgQueue_t *mq);

/*
 * PreAccept * p = *(PreAccept **)getNextMsg(mq);
 */

void putIntoMsgQueue(MsgQueue_t * mq, void * src);

/*
 * PreAccept * p;      -- pointer of a PreAccept msg (received from rdma) to be put into msgQueue
 * putIntoMsgQueue(mq, &p);
 */


#endif