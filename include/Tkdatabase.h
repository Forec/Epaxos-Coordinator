//
// Created by jingle on 17-1-3.
//




#include <stdint-gcc.h>
#include <iostream>
#include <string.h>
#include <set>
#include <string>
//#include <ext/hash_map>
//#include <ext/hash_set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "../c/generated/zookeeper.jute.h"

using namespace std;

using namespace __gnu_cxx;


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
//    int dataLength;
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




struct Tkdatabase {

    unordered_map <string, datanode_t*> Tkdb;

    unordered_map <uint64_t, unordered_set<string>> ephemerals;

};

typedef struct Tkdatabase Tkdatabase_t;




void init_tkdatabase(Tkdatabase_t *db);

void destroy_tkdatabase(Tkdatabase_t *db);

string parse_path_child(const string & node_path);

string parse_path_parent(const string & node_path);

int add_node_to_db(Tkdatabase_t* db, datanode_t *datanode, const string & node_path);

int getdata_from_db(const Tkdatabase_t *db, const string & node_path, char* & getdata);

int del_node_in_db(Tkdatabase_t *db, const string &node_path);

int getchildren_from_db(Tkdatabase_t *db, const string & path, unordered_set<string> & res);

int setData_to_datanode(Tkdatabase_t *db, const string & path, char *data);
