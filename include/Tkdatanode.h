//
// Created by jingle on 17-1-4.
//

#ifndef TKDATABASE_TKDATANODE_H
#define TKDATABASE_TKDATANODE_H

#include <string>
#include <unordered_set>

using namespace std;


//struct Stat {
//
//    uint64_t ctxid; //创建的txid;
//    uint64_t mtxid;// 最新修改txid;
//    uint64_t  ctime; //创建时间;
//    uint64_t mtime; //修改时间;
//    int  version;  //对应的Znode data;
//    int cversion;
//    int aversion;
//    uint64_t ephemeralOwner; // 临时节点对应的client session id, 默认为0;
//    uint64_t dataLength;
//    int numClidren;
//    uint64_t  ptxid; // 最后一次修改子节点个数的txid;
//
//};
typedef struct Stat Stat_t;

typedef struct datanode datanode_t;
struct datanode{
    datanode_t* parent;
    //uint32_t acl;
    char * data;
    unordered_set <string> children;
    //Stat_t stat;
};



void addchild(datanode_t *root, const string &child);

void removechild(datanode_t *root, const string &child);



void setChildren(datanode_t *root, const unordered_set<string>& children);



unordered_set<string> getChildren(datanode_t *root);


#endif //TKDATABASE_TKDATANODE_H
