//
// Created by forec on 17-5-1.
//

#include "../include/utils.h"

void nano_sleep(uint64_t ns){
    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = ns;
    nanosleep(&ts, 0);
}

bool isInitialBallot(int32_t ballot) {
    return (ballot >> 4) == 0;
}

bool equal(std::array<int32_t, GROUP_SZIE> & deps1,
           std::array<int32_t, GROUP_SZIE> & deps2) {
    for (int i = 0; i < deps1.size(); i++) {
        if (deps1[i] != deps2[i])
            return false;
    }
    return true;
}