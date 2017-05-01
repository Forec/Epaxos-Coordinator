//
// Created by forec on 17-5-1.
//

#include "../include/utils.h"

void nano_sleep(uint64_t ns){
    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = ns;
    nanosleep(&ts, 0);
}

