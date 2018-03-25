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
//typedef FileHeader FileHeader_t;

using namespace std;

struct FileList{
    int32_t len;
    int32_t* fdList;
};

typedef FileList FileList_t;

struct TxnLog
{
	struct TxnHeader* hdr;
	struct buffer * log;
};

typedef TxnLog TxnLog_t;

struct FileTxnLog
{
	int64_t preAllocSize;
	int32_t version;
	int64_t dbId;
	string fileName;
//	char* fileName;
	char* toFlush;
	int64_t currentSize;
	int64_t currentPosition;
	char* log;
	std::vector<TxnLog_t> txnList;
};

typedef FileTxnLog FileTxnLog_t;

void readLogFromFile(int32_t snapshotZxid);
void readDataBaseFromFile(string fileName);
void writeDataBaseToFile();
int32_t serializeDataNode(dataNode node,struct oarchive *out);
dataNode deserializeDataNode(struct iarchive *in);
void initFileTxnLog(FileTxnLog_t* FTL,int32_t version);
bool append(FileTxnLog_t *FTL,TxnLog_t* txn);
uint32_t Checksum(unsigned char *data, int32_t len);
bool fileNameCompare(string name1,string name2);
vector<string> readFileList();
bool readNextLogFile(vector<string> fileName,struct readBuff* rb);
int file_size2(string filename);

#endif
