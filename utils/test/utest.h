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
#include <thread>
//#include <string>
#include <array>
#include <vector>
//#include "../include/utils.h"
#include "../include/msg_queue.h"
//#include "../include/go_thread.h"
//#include "../include/communication.h"

struct Test_Struct {
    int id;
    int count;
    int buf_len;
    char * buf;
};

#define TEST_SIZE 200
#define MSG_SIZE 8

struct mq_timer {
    ev_timer timer;
    MsgQueue * mq;
    Test_Struct * p;
};

struct param {
    MsgQueue * mq;
    Test_Struct * test_structs;
};

static void readMsgQueue_cb(EV_P_ ev_timer *w, int r);
void readMsgQueue_thread(MsgQueue * mq, Test_Struct * src);
void putMsgQueue_thread(MsgQueue * mq, Test_Struct * src);
void * communication_thread(void * arg);
void * listen_thread(void * arg);

#endif
