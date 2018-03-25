//
// Created by forec on 17-5-8.
//

#ifndef TKDATABASE_ZOOKEEPERSERVER_H
#define TKDATABASE_ZOOKEEPERSERVER_H

#include <string>
#include <vector>
#include <chrono>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "../../config/config.h"
#include "../../consensus/include/tk_command.h"
#include "../../consensus/include/tk_message.h"
#include "../../utils/include/msg_queue.h"
#include "../../coor_log/include/tk_txn.h"
#include "zk_adaptor.h"
#include "serverCnxn.h"
#include "dataTree.h"

#define maxConnections 100
#define RequestHeaderLength 8

struct zookeeperServer {
    int32_t Listener;
    int64_t zxid;
    int32_t serverId;
    int64_t packetsSent;
    int64_t packetsReceived;
    int64_t maxLatency;
    int64_t minLatency ;
    int64_t totalLatency;
    int64_t count;
    serverCnxnFactory cnxnFactory;
    buffer_head_t pre_process; /*The buffer that have been read and are ready to be pre-processed*/
    buffer_head_t to_process; /* The buffers that have been pre-porcessed and are ready to be processed. */
    buffer_head_t to_send; /* The packets queued to send */
    dataTree zoo_dataTree;
    struct epoll_event events[maxConnections];
};


bool initZookeeperServer(zookeeperServer * zServer,int32_t listenPort,int32_t serverId);
bool ping(int sock);
void pre_processRequest(struct zookeeperServer * zServer,buffer_list_t * toProcess,MsgQueue * pro_mq);
void processRequest(struct zookeeperServer * zServer,buffer_list_t * toProcess);
void processConnectRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int32_t socketfd,MsgQueue * pro_mq);

int64_t getNextZxid(struct zookeeperServer * zServer);
int64_t getTime();
void setnonblocking(int sock);
void listenOnPort(zookeeperServer * zkServer,MsgQueue * pro_mq);
void pre_processPacket(zookeeperServer * zkServer,MsgQueue * pro_mq);
void processPacket(zookeeperServer * zkServer);
void sendResponse(zookeeperServer * zkServer);
void removeSession(int64_t sessionId,zookeeperServer * zkServer);

#endif //TKDATABASE_ZOOKEEPERSERVER_H
