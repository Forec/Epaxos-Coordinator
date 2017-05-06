//
// Created by forec on 17-5-1.
//

#ifndef TKDATABASE_UTILS_H_H
#define TKDATABASE_UTILS_H_H

#include <time.h>
#include <stdint.h>
#include <array>
#include "../../config/config.h"

void nano_sleep(uint64_t ns);
bool isInitialBallot(int32_t ballot);
bool equal(std::array<int32_t, GROUP_SZIE> & deps1, std::array<int32_t, GROUP_SZIE> & deps2);

#endif //TKDATABASE_UTILS_H_H
