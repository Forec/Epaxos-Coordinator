//
// Created by forec on 17-5-8.
//

#ifndef TKDATABASE_SERVER_H
#define TKDATABASE_SERVER_H

#include "../../config/config.h"
#include "../../consensus/include/tk_message.h"
#include "../../consensus/include/replica.h"
#include <string>
#include <stdint.h>
#include <thread>
#include <chrono>
#include <vector>

struct Server {
    Replica * r;
    int32_t Id;
    std::vector<std::string> addrList;
    std::vector<int> portList;
    Server(){};
    Server(const Server & another) {
        r = another.r;
        addrList = another.addrList;
        portList = another.portList;
    }
};

void registerWithMaster(const std::string & addr, int port, Server & s);
// This function will block the main thread of server if master not ready.

void handleMasterMessages(int sock);
// This is an independent thread, communicates with master.

#endif //TKDATABASE_SERVER_H
