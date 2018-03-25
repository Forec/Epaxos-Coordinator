//
// create by wuhao
//

#ifndef ZKDATABASE_LOG
#define ZKDATABASE_LOG

#include <vector>
#include <string>
#include "../include/zookeeperServer.jute.h"
#include "../include/zookeeperServer.recordio.h"
#include "../include/dataTree.h"

using namespace std;

struct LogHeader
{
	int64_t sessionId;
	int32_t cxid;
	int64_t zxid;
	int64_t time;
	int32_t type;
	string status;
};

typedef LogHeader LogHeader_t;

struct TxnLog
{
	LogHeader_t hdr;
	struct buffer log;
};

typedef TxnLog TxnLog_t;

struct FileTxnLog
{
	int64_t preAllocSize;
	int32_t version;
	int64_t dbId;
	string fileName;
	char* toFlush;
	int64_t currentSize;
	int64_t currentPosition;
	char* log;
	std::vector<TxnLog_t> txnList;
};

typedef FileTxnLog FileTxnLog_t;

struct readBuff{
    int32_t len;
    int32_t position;
    char* buff;
    vector<string> fileName;
};

struct readBuff readLogFromFile(int32_t snapshotZxid);
void initFileTxnLog(FileTxnLog_t* FTL,int32_t version);
bool append(FileTxnLog_t *FTL,TxnLog_t* txn);
uint32_t Checksum(unsigned char *data, int32_t len);
bool fileNameCompare(string name1,string name2);
vector<string> readFileList();
bool readNextLogFile(struct readBuff* rb);
int file_size2(string filename);
int32_t nextLog(TxnLog_t* txn,int32_t snapshotId,struct readBuff*rb);
//serialize & deserialize txnheader
void serializeTxnHeader(LogHeader_t* pat,struct serializeBuff* oa);
void deserializeTxnHeader(struct serializeBuff* ia,LogHeader_t* pat);

#endif
