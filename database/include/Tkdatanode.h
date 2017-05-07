//
// Created by jingle on 17-1-4.
// Refactored by forec on 17-5-7.
//

#ifndef TKDATABASE_TKDATANODE_H
#define TKDATABASE_TKDATANODE_H

#include <string>
#include <unordered_set>

struct Datanode{
    Datanode * parent;
    int32_t dataSize;
    char * data;
    std::unordered_set<std::string> children;
    void addChild(const std::string &child);
    void removeChild(const std::string &child);
    void setChildren(const std::unordered_set<std::string>& children);
    std::unordered_set<std::string> getChildren();
};

// TODO
struct NodeStatus {
    int      version;
    int      cversion;
    int      aversion;
    int      numClidren;
    uint64_t ctxid;           // 创建的 txid;
    uint64_t mtxid;           // 最新修改 txid;
    uint64_t ctime;           // 创建时间;
    uint64_t mtime;           // 修改时间;
    uint64_t ephemeralOwner;  // 临时节点对应的client session id, 默认为0;
    uint64_t ptxid;           // 最后一次修改子节点个数的txid;
};

#endif //TKDATABASE_TKDATANODE_H
