//
// Created by forec on 17-5-8.
//

#ifndef TKDATABASE_MASTER_H
#define TKDATABASE_MASTER_H

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include "../../config/config.h"
#include "../../consensus/include/tk_message.h"

struct Master {
    int N;
    int Listener;
    std::vector<std::string> nodeList;
    std::vector<std::string> addrList;
    std::vector<int>         portList;
    std::mutex               lock;
    std::vector<RDMA_CONNECTION> nodes;
    std::vector<bool>        leader;
    std::vector<bool>        alive;
};

void waitingConnections(Master * master);
void handlers(Master * master, int nsock);
void handleRegister(Master * master, int nsock);
void handleLeaderArgs(Master * master, int nsock);
void handleReplicaListArgs(Master * master, int nsock);
bool ping(int sock);
bool BeTheLeader(int sock);

#endif //TKDATABASE_MASTER_H
