//
// Created by forec on 17-4-27.
//

#ifndef __UTEST_H_
#define __UTEST_H_

#include <ev.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <thread>
#include <string>
#include <array>
#include <vector>
#include <chrono>
#include "../include/msg_queue.h"
#include "../include/communication.h"

struct Test_Struct {
    int id;
    int count;
    int buf_len;
    char * buf;
    Test_Struct(){};
    Test_Struct(const Test_Struct & another) {
        id = another.id;
        count = another.count;
        buf_len = another.buf_len;
        buf = new char[buf_len + 1];
        mempcpy(buf, another.buf, buf_len);
    }
    ~Test_Struct() {
        delete [] buf;
    }
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
void communication_thread(int idx);
void listen_thread(int sock);

extern std::atomic<uint32_t> passed;
extern std::atomic<uint32_t> failed;
void alert(bool conf) {
    if (conf)
        passed++;
    else
        failed++;
};

#endif
