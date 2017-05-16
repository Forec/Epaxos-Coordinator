//
// Created by forec on 17-5-8.
//

#ifndef TKDATABASE_CLIENT_H
#define TKDATABASE_CLIENT_H

#include "../../utils/include/utils.h"
#include "../../utils/include/msg_queue.h"
#include "../../consensus/include/tk_message.h"
#include <vector>
#include <string>
#include <array>
#include <thread>
#include <chrono>
#include <ctime>
#include <unordered_map>

#define CONFLICT_KEY "/CONFLICT"
#define TEST_KEY "/TEST"
#define MAX_VALUE_SIZE 11

std::string randStr(int least = 0, int most = MAX_VALUE_SIZE);
void waitReplies(std::vector<RDMA_CONNECTION> & servers, int leader, int n, MsgQueue * mq);

#endif //TKDATABASE_CLIENT_H
