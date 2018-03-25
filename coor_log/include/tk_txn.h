//
// create by wuhao
//

#ifndef ZKDATABASE_LOG
#define ZKDATABASE_LOG

#include <vector>
#include <string>
#include "../../consensus/include/tk_elog.h"
#include "../../consensus/include/tk_command.h"
#include "../../zookeeperServer/include/zookeeperServer.h"
#include "../../zookeeperServer/include/dataTree.h"

using namespace std;

struct serializeBuff{
    int32_t len;
    char* buff;
};

struct readBuff{
    int64_t len;
    int64_t position;
    char* buff;
    vector<string> fileName;
};

bool writeDataTreeToFile(dataTree * zoo_dataTree);
bool writeDataNodeToFile(dataNode * node,int fd);

bool readDataTreeFromFile(dataTree * zoo_dataTree,string fileName,int32_t fd);

struct readBuff readInstanceFromFile(int32_t snapshotZxid);
bool append(int32_t replicaId,struct tk_instance ti);
uint32_t Checksum(unsigned char *data, int32_t len);
bool readNextLogFile(struct readBuff* rb);
int file_size2(string filename);
int32_t nextInstance(struct tk_instance* pi,struct readBuff* rb);

//serialize instance
void serializeInstance(struct tk_instance pi,struct serializeBuff* oa);

#endif
