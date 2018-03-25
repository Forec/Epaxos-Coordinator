//
//	create by wuhao
//

#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <dirent.h>
#include "../include/snapshot.h"
#include "../include/zookeeperServer.jute.h"

using namespace std;
#define MOD_ADLER 65521
#define basePath "./"

#define dataBaseFileName "database"
//定义flags:只写，文件不存在那么就创建，文件长度戳为0
#define FLAGS O_WRONLY | O_CREAT | O_TRUNC
//创建文件的权限，用户读、写、执行、组读、执行、其他用户读、执行
#define MODE S_IRWXU | S_IXGRP | S_IROTH | S_IXOTH





void int2str(const int &int_temp,string &string_temp)
{
        stringstream stream;
        stream<<int_temp;
        string_temp=stream.str();   //此处也可以用 stream>>string_temp
}
uint32_t Checksum(unsigned char *data, int32_t len){
	uint32_t a = 1, b = 0;
    /* Loop over each byte of data, in order */
    for (size_t index = 0; index < len; ++index){
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    return (b << 16) | a;
}

bool writeDataBaseToFile(dataTree dt){
    int fd = 0;
    if ((fd = open(dataBaseFileName, FLAGS, MODE)) == -1) {
        printf("open file error");
        return false;
    }
    //写入每一个datanode

    return true;
}

int32_t serializeDataNode(dataNode node,struct oarchive *out){
    int rc;
    //path : string
    //stat : statPersisted
    //acl : aclvector
    //data : buffer
    char * path = (char *)node.path.c_str();
    rc = out->serialize_String(out, "",& path);
    serialize_StatPersisted(out,"",&node.stat);
    serialize_ACL_vector(out,"",&node.acl);
    rc = out->serialize_Buffer(out, "", &node.data);
    return rc;
}

dataNode deserializeDataNode(struct iarchive *in){
    dataNode node;
    char * path;
    in->deserialize_String(in,"",&path);
    node.path = path;
    deserialize_StatPersisted(in,"",&node.stat);
    deserialize_ACL_vector(in,"",&node.acl);
    in->deserialize_Buffer(in,"",&node.data);
    return node;
}

void initFileTxnLog(FileTxnLog_t* FTL,int32_t version){
	FTL->preAllocSize = 65536*1024;
	FTL->version = version;
	FTL->fileName ="";
	FTL->currentSize = 65536*1024;
	FTL->currentPosition = 0;
	FTL->toFlush = (char*)malloc(FTL->currentSize);
	memset(FTL->toFlush,0,FTL->currentSize);
}

bool append(FileTxnLog_t *FTL,TxnLog_t* txn){
    int fd = 0;
    if(FTL->fileName==""){
        string num = "";
        int2str(txn->hdr.zxid,num);
        FTL->fileName = "log."+num;
        fd = creat(FTL->fileName.c_str(),S_IRWXU);
        if(fd < 0)
            return false;
    }
    fd = open(FTL->fileName.c_str(),O_WRONLY|O_APPEND);
    struct buffer sb ;
    serializeTxnHeader(&txn->hdr,&sb);
    int32_t txnHeaderLen = sb.len;
    int32_t logLen = txn->log.len;
    unsigned char* checkSumCharArray = (unsigned char*)malloc(txnHeaderLen+logLen);
    memcpy(checkSumCharArray,sb.buff,txnHeaderLen);
    memcpy(checkSumCharArray+txnHeaderLen,txn->log.buff,logLen);
    uint32_t checksum = Checksum(checkSumCharArray,txnHeaderLen+logLen);
    int wlen = 0;
    wlen = write(fd,&checksum,sizeof(uint32_t));//checksum
    wlen = write(fd,&txnHeaderLen,sizeof(int32_t));//header length
    wlen = write(fd,sb.buff,sb.len);//header
    wlen = write(fd,&txn->log.len,sizeof(int32_t));//log length
    wlen = write(fd,txn->log.buff,logLen);//log
    char logEnd = 0x42;
    wlen = write(fd,&logEnd,sizeof(char));
    int temp = close(fd);
	return true;
}

int padFile(FileTxnLog_t *FTL,int fd){
	char *buffer= NULL;
	if(FTL->currentPosition + 4096 >= FTL->currentSize){
		FTL->currentSize += FTL->preAllocSize;
		buffer = (char*)realloc(FTL->toFlush, FTL->currentSize);
    	if (!buffer) {
        	FTL->toFlush = 0;
        	return -ENOMEM;
    	}
   		FTL->toFlush = buffer;
	}
	return 0;
}

void writeToFlush(FileTxnLog_t* FTL,char byte[],int32_t len){
	memcpy(FTL->toFlush+FTL->currentPosition,byte,len);
	FTL->currentPosition += len;
}
bool sortDataDir(int64_t snapshotZxid,struct readBuff* rb){
    vector<string> fileNameListVector = readFileList();
    //vector<string> fileNameList;
    sort(fileNameListVector.begin(),fileNameListVector.end(),fileNameCompare);
    string num1;
    int firstLogFilePosition = 0;
    for(int i = 0;i < fileNameListVector.size();i++){
        num1.assign(fileNameListVector[i],4,fileNameListVector[i].length()-3);
        int zxid = atoi(num1.c_str());
        if(zxid > snapshotZxid){
            firstLogFilePosition = i-1;
            break;
        }
    }
    for(int i = firstLogFilePosition;i < fileNameListVector.size();i++){
        rb->fileName.push_back(fileNameListVector[i]);
        //cout<<fileNameListVector[i]<<endl;
    }
    vector<string>().swap(fileNameListVector);
    return true;
}

bool truncate(int64_t Zxid){
    vector<string> fileNameListVector = readFileList();
    int firstLogFilePosition = 0;
    string num1;
    for(int i = 0;i < fileNameListVector.size();i++){
        num1.assign(fileNameListVector[i],4,fileNameListVector[i].length()-3);
        int zxid = atoi(num1.c_str());
        if(zxid > Zxid){
            firstLogFilePosition = i;
            break;
        }
    }
    return false;
}
vector<string> readFileList(){
    vector<string> fileNameListVector;
    DIR *dir;
    struct dirent *ptr;
    if ((dir=opendir(basePath)) == NULL){
        perror("Open dir error...");
        exit(1);
    }
    while ((ptr=readdir(dir)) != NULL){
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            string temp = ptr->d_name;
            if(temp.substr(0,4)=="log."){
                fileNameListVector.push_back(temp);
            }
        }
    }
    closedir(dir);
    return fileNameListVector;
}


bool formatTxn(TxnLog_t* txn,struct readBuff* rb){
    //checksum
    uint32_t checksumRead = 0;
    memcpy(&checksumRead,rb->buff+rb->position,sizeof(uint32_t));
    rb->position+=sizeof(uint32_t);
    //headerlen
    int32_t headerLen = 0;
    memcpy(&headerLen,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    //header
    char header[headerLen];
    memcpy(&header,rb->buff+rb->position,headerLen);
    rb->position+=headerLen;
    //loglen
    int32_t logLen = 0;
    memcpy(&logLen,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    //log
    char log[logLen];
    memcpy(&log,rb->buff+rb->position,logLen);
    rb->position+=logLen;
    //验证checksum
    unsigned char* check = (unsigned char*)malloc(headerLen+logLen);
    memcpy(check,header,headerLen);
    memcpy(check+headerLen,log,logLen);
    uint32_t checksumCal = Checksum(check,headerLen+logLen);
    if(checksumCal != checksumRead){
        cout<<"读出来的ｃｈｅｃｋｓｕｍ"<<checksumRead<<endl;
        cout<<"计算出来的ｃｈｅｃｋｓｕｍ"<<checksumCal<<endl;
        cout<<"checksum is wrong"<<endl;
        return false;
    }
    int32_t record = 0;
    memcpy(&record,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    char logEnd = 'a';
    memcpy(&logEnd,rb->buff+rb->position,sizeof(char));
    rb->position+=sizeof(char);
    if(logEnd != 0x42){
        cout<<"没有以Ｂ结束"<<endl;
        return false;
    }
    LogHeader_t txnHeader;
    struct serializeBuff sb;
    sb.len = headerLen;
    sb.buff = &header;
    deserializeTxnHeader(&sb,&txnHeader);
    txn->hdr.sessionId = txnHeader.sessionId;
    txn->hdr.cxid = txnHeader.cxid;
    txn->hdr.zxid = txnHeader.zxid;
    txn->hdr.time = txnHeader.time;
    txn->hdr.type = txnHeader.type;
    txn->hdr.status = txnHeader.status;
    txn->log.buff = (char*)malloc(logLen);
    memcpy(txn->log.buff,log,logLen);
    txn->log.len = logLen;
    txn->recordtype = record;
    return true;
}

struct readBuff readLogFromFile(int32_t snapshotZxid){
    struct readBuff rb;
    rb.len = 0;
    rb.position = 0;
    sortDataDir(snapshotZxid,&rb);
    return rb;
}

int32_t nextLog(Txn_t* txn,int32_t snapshotId,struct readBuff*rb){
    if(rb->len > rb->position){
        //cout<<"len"<<rb->len<<"position"<<rb->position<<endl;
        bool flag = formatTxn(txn,rb);
        if(!flag){
            return 2;//解析错误
        }
        if(txn->hdr.zxid >= snapshotId){
            return 1;//成功
        }else {
            return nextLog(txn,snapshotId,rb);
        }
    }else if(rb->len==rb->position){
        //cout<<"len==position"<<rb->position<<endl;
        if(readNextLogFile(rb)){
            return nextLog(txn,snapshotId,rb);
        }else{
            return 3;//结束
        }
    }
}

bool readNextLogFile(struct readBuff* rb){
    if(rb->fileName.size()>=1){
        //cout<<rb->fileName[0]<<endl;
        int fd = open(rb->fileName[0].c_str(),O_RDONLY);
        if(rb->position!=0){
            free(rb->buff);
        }
        rb->len = file_size2(rb->fileName[0]);
        rb->buff = (char*)malloc(rb->len);
        rb->position = 0;
        int i = read(fd,rb->buff,rb->len);
        close(fd);
        if(i != -1){
            rb->fileName.erase(rb->fileName.begin());
            return true;
        }
    }
    return false;
}

int32_t file_size2(string filename)
{
    struct stat statbuf;
    stat(filename.c_str(),&statbuf);
    int32_t size=statbuf.st_size;
    return size;
}

bool fileNameCompare(string name1,string name2){
    string num1;
    num1.assign(name1,4,name1.length()-3);
    string num2;
    num2.assign(name2,4,name2.length()-3);
    int zxid1 = atoi(num1.c_str());
    int zxid2 = atoi(num2.c_str());
    return zxid1 < zxid2;
}

/*void serializeTxnHeader(TxnHeader_t *pat,struct serializeBuff* oa){

	tkJute::TxnHeaderJute pa;
    pa.set_sessionid(pat->sessionId);
    pa.set_cxid(pat->cxid);
    pa.set_zxid(pat->zxid);
    pa.set_time(pat->time);
    pa.set_type(pat->type);
    pa.set_status(pat->status);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeTxnHeader(struct serializeBuff* ia,TxnHeader_t *pat){

	tkJute::TxnHeaderJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->cxid = pa.cxid();
        pat->sessionId = pa.sessionid();
        pat->zxid = pa.zxid();
        pat->time = pa.time();
        pat->type = pa.type();
        pat->status = pa.status();
	}else{
        cout<<"deserilizeTxnHeader fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}*/
