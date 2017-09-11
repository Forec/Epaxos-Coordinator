//
// Created by haoba on 17-7-21.
//

#ifndef TKDATABASE_SESSION_H
#define TKDATABASE_SESSION_H


#include <map>
#include <vector>
#include <pthread.h>
#include <set>
#include "zookeeperServer.recordio.h"
#include "zookeeperServer.jute.h"
#include "proto.h"

using namespace std;

typedef struct {
    int64_t sessionId;
    int32_t sessionTimeout;
    char password[16];
    int64_t tickTime;
    bool isClosing;
    int64_t owner;
}Session;

struct serverCnxn_t{

    int32_t socketfd;
    int64_t sessionId;
    vector<struct Id> auth;
};

typedef serverCnxn_t serverCnxn;

struct sessionTracker_t{
    int64_t nextSessionId;
    int64_t serverId;
    map<int64_t ,Session> sessionById;
    map<int64_t ,int32_t > sessionWithTimeOut;
    map<int64_t ,set<Session>> sessionSets;
    int32_t expirationInterval;
    int64_t nextExpirationTime;
    pthread_mutex_t lock;
};

typedef sessionTracker_t sessionTracker;

struct serverCnxnFactory_t{
    int32_t tick;
    int32_t minSessionTimeout;
    int32_t maxSessionTimeout;
    map<int64_t ,serverCnxn> cnxnList;
    map<int32_t ,int64_t > cnxnIdBySocketfd;
    sessionTracker st;
    pthread_mutex_t lock;
};

typedef serverCnxnFactory_t serverCnxnFactory;

int64_t createSessionId(sessionTracker * st,int32_t timeout);
void getPassword(char *str,int64_t sessionId);
int64_t createCnxn(serverCnxnFactory * cnxnFactory,int32_t socketfd,int32_t timeout);
int64_t createSession(sessionTracker * st,int32_t timeout);
ZOO_ERRORS zoo_addAuth(serverCnxn * cnxn,struct AuthPacket authPacket);
bool addSession(sessionTracker * st,int64_t sessionId,int32_t timeout);
bool touchSession(int64_t sessionId,int32_t timeout,sessionTracker * st);
sessionTracker initSessinTracker(int64_t nextSessionId,int32_t tickTime,map<int64_t ,int32_t > sessionWithTimeOut);
void setSessionClosing(int64_t sessionId,sessionTracker * st);
void processSessionTracker(sessionTracker * st);
bool operator < (const Session & a,const Session &b);
int32_t lockSessionTracker(sessionTracker * st);
int32_t unLockSessionTracker(sessionTracker * st);
ZOO_ERRORS checkSession(int64_t sessionId,sessionTracker * st,int64_t serverId);
ZOO_ERRORS setOwner(int64_t sessionId,sessionTracker * st,int64_t serverId);

#endif //TKDATABASE_SESSION_H
