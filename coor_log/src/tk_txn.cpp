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
#include <vector>
#include <dirent.h>
#include "../include/tk_txn.h"
#include "../../serialization/include/tk_jute.h"

using namespace std;
#define MOD_ADLER 65521
#define basePath "./"


struct readBuff{
    int32_t len;
    int32_t position;
    char* buff;
};

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

void initFileTxnLog(FileTxnLog_t* FTL,int32_t version){
	FTL->preAllocSize = 65536*1024;
	FTL->version = version;
	FTL->fileName ="";
	FTL->currentSize = 65536*1024;
	FTL->currentPosition = 0;
	FTL->toFlush = (char*)malloc(FTL->currentSize);
	memset(FTL->toFlush,0,FTL->currentSize);
}

bool append(FileTxnLog_t *FTL,Txn_t* txn){
    int fd = 0;
	if(txn->hdr != NULL){
        if(FTL->fileName==""){
            string num = "";
            int2str(txn->hdr->zxid,num);
            FTL->fileName = "log"+num;
            fd = creat(FTL->fileName.c_str(),S_IRWXU);
            if(fd < 0)
                return false;
        }
        fd = open(FTL->fileName.c_str(),O_WRONLY|O_APPEND);
		int32_t txnHeaderLen = 0;
		int32_t logLen = txn->log->len;
		unsigned char* checkSumCharArray = (unsigned char*)malloc(txnHeaderLen+logLen);
		memcpy(checkSumCharArray,txn->hdr,txnHeaderLen);
		memcpy(checkSumCharArray+txnHeaderLen,txn->log->buff,logLen);
		uint32_t checksum = Checksum(checkSumCharArray,txnHeaderLen+logLen);
		txnHeaderLen = 64+txn->hdr->status.length()+1;
		int wlen = 0;
		wlen = write(fd,&checksum,sizeof(uint32_t));//checksum
		wlen = write(fd,&txnHeaderLen,sizeof(int32_t));//header length
		wlen = write(fd,txn->hdr,txnHeaderLen);//header
		wlen = write(fd,&txn->log->len,sizeof(int32_t));//log length
		wlen = write(fd,txn->log->buff,logLen);//log
		wlen = write(fd,&txn->recordtype,sizeof(int32_t));//record type
		char logEnd = 0x42;

		int temp = close(fd);
		fd = open(FTL->fileName.c_str(),O_RDONLY);
		char result[wlen];
		int rlen = read(fd,&result,wlen);
        int id = 0;
        memcpy(&id,result,8);
        cout<<id;

	}
	return false;
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
void sortDataDir(int64_t snapshotZxid){
    vector<string> fileNameListVector = readFileList();
    vector<string> fileNameList;
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
        fileNameList.push_back(fileNameListVector[i]);
        cout<<fileNameListVector[i]<<endl;
    }
    vector<string>().swap(fileNameListVector);
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


Txn_t formatTxn(readBuff* rb){
    Txn_t txn;
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
    unsigned char check[headerLen+logLen];
    memcpy(check,header,headerLen);
    memcpy(check+headerLen,log,logLen);
    uint32_t checksumCal = Checksum(check,headerLen+logLen);
    if(checksumCal != checksumRead){
        cout<<"checksum is wrong"<<endl;
        return txn;
    }
    int32_t record = 0;
    memcpy(&record,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    char logEnd = 'a';
    memcpy(&logEnd,rb->buff+rb->position,sizeof(char));
    rb->position+=sizeof(char);
    if(logEnd != 0x42){
        cout<<"没有以Ｂ结束"<<endl;
        return txn;
    }
    int headerPosition = 0;
    TxnHeader_t txnHeader;
    memcpy(&(txnHeader.clientId),header+headerPosition,sizeof(int64_t));
    headerPosition+=sizeof(int64_t);
    memcpy(&(txnHeader.cxid),header+headerPosition,sizeof(int32_t));
    headerPosition+=sizeof(int32_t);
    memcpy(&(txnHeader.zxid),header+headerPosition,sizeof(int64_t));
    headerPosition+=sizeof(int64_t);
    memcpy(&(txnHeader.time),header+headerPosition,sizeof(int64_t));
    headerPosition+=sizeof(int64_t);
    memcpy(&(txnHeader.type),header+headerPosition,sizeof(int32_t));
    headerPosition+=sizeof(int32_t);
    memcpy(&(txnHeader.status),header+headerPosition,headerLen-headerPosition);

    txn.hdr = &txnHeader;
    struct serializeBuff sbuff;
    memcpy(sbuff.buff,log,logLen);
    sbuff.len = logLen;
    txn.log = &sbuff;
    txn.recordtype = record;
    return txn;
}

void readLogFromFile(int32_t snapshotZxid){
    sortDataDir(snapshotZxid);

}

Txn_t nextLog(struct readBuff* rb,vector<string> fileName){
    if(rb->len > rb->position){
        return formatTxn(rb);
    }else if(rb->len==rb->position){
        readNextLogFile(fileName,rb);
        nextLog(rb,fileName);
    }
}

bool readNextLogFile(vector<string> fileName,struct readBuff* rb){
    if(fileName.size()>=1){
        //FILE* fp;
        //fp=fopen("test.txt","rb");
        int fd = open(fileName[0].c_str(),O_RDONLY);
        if(rb->buff != NULL){
            free(rb->buff);
            rb->len = file_size2(fileName[0]);
            rb->buff = (char*)malloc(rb->len);
            rb->position = 0;
        }
        int i = read(fd,rb->buff,rb->len);
        if(i != -1)
        return true;
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
