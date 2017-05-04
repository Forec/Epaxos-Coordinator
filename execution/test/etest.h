//
// Created by forec on 17-4-22.
//

#ifndef TKDATABASE_ETEST_H
#define TKDATABASE_ETEST_H

#define CATCH_CONFIG_MAIN

#include <ev.h>
#include <catch.hpp>
#include <pthread.h>
#include "../../consensus/include/replica.h"
#include "../include/exec.h"
#include <vector>
#include <string>

struct my_timer {
    ev_timer timer;
    union {
        tk_instance * instance;
        Replica * replica;
    };
};

typedef struct my_timer my_timer_t;

static void commit_timeout_cb(EV_P_ ev_timer *w, int r);
static void shutdown_timeout_cb(EV_P_ ev_timer *w, int r);
void * execute_thread_t(void * arg);

#endif //TKDATABASE_ETEST_H
