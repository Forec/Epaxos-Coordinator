//
// Created by haboa on 17-8-28.
//

#ifndef ZOOKEEPERSERVER_WATCH_MANAGER_H
#define ZOOKEEPERSERVER_WATCH_MANAGER_H

#include "zookeeperServer.jute.h"
#include "zk_adaptor.h"
#include "proto.h"
#include <unordered_map>
#include <set>
#include <string.h>
#include <hash_fun.h>

using namespace std;

typedef struct {
    int64_t sessionId;
}watch;

typedef struct {
    unordered_map<string ,set<watch>> watchTable;
    unordered_map<int64_t ,set<string>> watch2Path;
}watchManager;

void addWatch(char * path,watchManager * manager,int64_t sessionId);

void triggerWatch(char * path,int32_t eventType,watchManager * manager,buffer_head_t * to_send);

void processEvent(struct WatcherEvent event,buffer_head_t * to_send,int32_t sessionId);
bool operator < (const watch &a,const watch &b);
bool operator == (const watch &a, const watch &b);

#endif //ZOOKEEPERSERVER_WATCH_MANAGER_H
