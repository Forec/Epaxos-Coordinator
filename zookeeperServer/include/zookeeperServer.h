//
// Created by forec on 17-5-8.
//

#ifndef TKDATABASE_ZOOKEEPERSERVER_H
#define TKDATABASE_ZOOKEEPERSERVER_H

#include <string>
#include <vector>
#include <chrono>
#include "../../config/config.h"
#include "zk_adaptor.h"
#include "serverCnxn.h"
#include "dataTree.h"


struct zookeeperServer {
    int32_t Listener;
    int64_t zxid;
    int64_t Id;
    int64_t packetsSent;
    int64_t packetsReceived;
    int64_t maxLatency;
    int64_t minLatency ;
    int64_t totalLatency;
    int64_t count;
    serverCnxnFactory cnxnFactory;
    buffer_head_t to_process; /* The buffers that have been read and are ready to be processed. */
    buffer_head_t to_send; /* The packets queued to send */
    dataTree zoo_dataTree;
};


bool ping(int sock);

void processConnectRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int32_t i);

int64_t getNextZxid(struct zookeeperServer * zServer);
int64_t getTime();
void processRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int32_t i);
void setnonblocking(int sock);
void listenOnPort(zookeeperServer * zkServer);
void processPacket(zookeeperServer * zkServer);
void sendResponse(zookeeperServer * zkServer);

#endif //TKDATABASE_ZOOKEEPERSERVER_H
