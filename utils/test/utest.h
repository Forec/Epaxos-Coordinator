//
// Created by forec on 17-4-27.
//

#ifndef __UTEST_H_
#define __UTEST_H_

#define CATCH_CONFIG_MAIN

#include <catch.hpp>
#include <ev.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include "../include/msg_queue.h"

struct TestStruct {
    int id;
    int count;
    int buf_len;
    char * buf;
};

typedef struct TestStruct Test_Struct_t;

#define TEST_SIZE 200
#define MSG_SIZE 8

struct mq_timer {
    ev_timer timer;
    MsgQueue_t * mq;
    Test_Struct_t * p;
};

typedef struct mq_timer mq_timer_t;

struct param {
    MsgQueue_t * mq;
    union {
        Test_Struct_t * test_structs;
    };
};

typedef struct param param_t;

static void readMsgQueue_cb(EV_P_ ev_timer *w, int r);
void * readMsgQueue_thread(void * arg);
void * putMsgQueue_thread(void * arg);

#endif
