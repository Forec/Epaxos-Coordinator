//
// create by wuhao
//

#ifndef ZKDATABASE_LOG
#define ZKDATABASE_LOG

#include <vector>
#include <string>
#include "tk_jute.h"
//typedef FileHeader FileHeader_t;

using namespace std;

struct writeBuff{
    int32_t len;
    char* buff;
};

struct FileList{
    int32_t len;
    int32_t* fdList;
};

typedef FileList FileList_t;

struct TxnHeader
{
	int64_t clientId;
	int32_t cxid;
	int64_t zxid;
	int64_t time;
	int32_t type;
	string status;
};

typedef TxnHeader TxnHeader_t;

struct Txn
{
	TxnHeader_t* hdr;
	struct serializeBuff* log;
	int32_t recordtype;
};

typedef Txn Txn_t;

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
	std::vector<Txn_t> txnList;
};

typedef FileTxnLog FileTxnLog_t;

void readLogFromFile(int32_t snapshotZxid);
void initFileTxnLog(FileTxnLog_t* FTL,int32_t version);
bool append(FileTxnLog_t *FTL,Txn_t* txn);
uint32_t Checksum(unsigned char *data, int32_t len);
bool fileNameCompare(string name1,string name2);
vector<string> readFileList();
bool readNextLogFile(vector<string> fileName,struct readBuff* rb);
int file_size2(string filename);

#endif
