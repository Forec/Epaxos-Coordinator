//
//  create by wuhao
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
#include "../include/tk_txn.h"
#include "../../consensus/include/tk_elog.h"
#include "../../consensus/include/tk_command.h"
#include "../../config/config.h"

using namespace std;
#define MOD_ADLER 65521
#define basePath "./"


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

bool append(int32_t replicaId,struct tk_instance ti){
    int fd = 0;
    string num = "";
    int2str(replicaId,num);
    string fileName = "store-replica-"+num;
    fd = open(fileName.c_str(),O_CREAT|O_RDWR|O_APPEND,S_IRWXU);
    if(fd < 0){
        cout<<"couldn't open file---"<<fileName<<endl;
        return false;
    }
    struct serializeBuff sb;
    serializeInstance(ti,&sb);
    int32_t instanceLen = sb.len;
    //int32_t logLen = txn->log.len;

    int wlen = 0;
    wlen = write(fd,sb.buff,instanceLen);//log
    //wlen = write(fd,&txn->recordtype,sizeof(int32_t));//record type
    int temp = close(fd);
    return true;
}


bool formatInstance(struct tk_instance* ti,struct readBuff* rb){
    int32_t checkPosition = rb->position;
    //length
    int32_t length = 0;
    memcpy(&length,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    //ballot
    int32_t ballot = 0;
    memcpy(&ballot,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    ti->ballot = ballot;
    //status
    int32_t status = 0;
    memcpy(&status,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    ti->status = (enum STATUS)status;
    //seq
    int32_t seq = 0;
    memcpy(&seq,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    ti->seq = seq;
    //deps
    for(int i = 0;i < GROUP_SZIE;i++){
        memcpy(&ti->deps.at(i),rb->buff+rb->position,sizeof(int32_t));
        rb->position+=sizeof(int32_t);
    }
    //cmds
    int32_t cmdSize = 0;
    memcpy(&cmdSize,rb->buff+rb->position,sizeof(int32_t));
    rb->position+=sizeof(int32_t);
    for(int i = 0;i < cmdSize;i++){
        int32_t op = 0;
        struct tk_command tc;
        memcpy(&op,rb->buff+rb->position,sizeof(int32_t));
        rb->position+=sizeof(int32_t);
        tc.opcode = (enum OP)op;
        int32_t keySize = 0;
        memcpy(&keySize,rb->buff+rb->position,sizeof(int32_t));
        rb->position+=sizeof(int32_t);
        char key[keySize+1] = "";
        memcpy(key,rb->buff+rb->position,keySize);
        rb->position+=keySize;
        key[keySize] = '\0';
        tc.key = key;
        int32_t valSize = 0;
        memcpy(&valSize,rb->buff+rb->position,sizeof(int32_t));
        rb->position+=sizeof(int32_t);
        tc.val = (char*)malloc(valSize);
        memcpy(tc.val,rb->buff+rb->position,valSize);
        rb->position+=valSize;
        ti->cmds.push_back(tc);
    }
    //checksum
    uint32_t checksumRead = 0;
    memcpy(&checksumRead,rb->buff+rb->position,sizeof(uint32_t));
    rb->position+=sizeof(uint32_t);
    //验证checksum
    unsigned char* checkSumCharArray = (unsigned char*)malloc(length+sizeof(int32_t));
    memcpy(checkSumCharArray,rb->buff+checkPosition,length+sizeof(int32_t));
    uint32_t checksumCal = Checksum(checkSumCharArray,length);
    if(checksumCal != checksumRead){
        cout<<"读出来的ｃｈｅｃｋｓｕｍ"<<checksumRead<<endl;
        cout<<"计算出来的ｃｈｅｃｋｓｕｍ"<<checksumCal<<endl;
        cout<<"checksum is wrong"<<endl;
        return false;
    }
    char logEnd = 'a';
    memcpy(&logEnd,rb->buff+rb->position,sizeof(char));
    rb->position+=sizeof(char);
    if(logEnd != 0x42){
        cout<<"没有以Ｂ结束"<<endl;
        return false;
    }
    return true;
}



struct readBuff readInstanceFromFile(int32_t replicaId){
    struct readBuff rb;
    rb.len = 0;
    rb.position = 0;
    string num = "";
    int2str(replicaId,num);
    string fileName = "store-replica-"+num;
    int fd = open(fileName.c_str(),O_RDONLY);
    rb.len = file_size2(fileName);
    rb.buff = (char*)malloc(rb.len);
    rb.position = 0;
    int i = read(fd,rb.buff,rb.len);
    close(fd);
    //sortDataDir(snapshotZxid,&rb);
    return rb;
}

int32_t nextInstance(struct tk_instance* pi,struct readBuff*rb){
    if(rb->len > rb->position){
        //cout<<"len"<<rb->len<<"position"<<rb->position<<endl;
        bool flag = formatInstance(pi,rb);
        if(!flag){
            return 2;//解析错误
        }else {
            return 1;
        }
    }else if(rb->len==rb->position){
        //cout<<"len==position"<<rb->position<<endl;
        return 3;//结束

    }
}

int32_t file_size2(string filename)
{
    struct stat statbuf;
    stat(filename.c_str(),&statbuf);
    int32_t size=statbuf.st_size;
    return size;
}

void serializeInstance(struct tk_instance ti,struct serializeBuff* oa){


    int32_t serizlizeLength = 0;
    int32_t length = 0;
    int32_t cmdCount = sizeof(int32_t);
    oa->len = sizeof(int32_t)*4+sizeof(int32_t)*GROUP_SZIE+cmdCount;
    for(int i = 0;i < ti.cmds.size();i++){
        oa->len+=sizeof(int32_t)+sizeof(int32_t)+ti.cmds.at(i).key.size()+sizeof(int32_t)+ti.cmds.at(i).valSize;
    }
    length = oa->len;
    oa->len+=sizeof(int32_t);
    oa->len+=sizeof(char);

    oa->buff = (char*)malloc(oa->len);
    memcpy(oa->buff+serizlizeLength,&length,sizeof(int32_t));
    serizlizeLength+=sizeof(int32_t);
    memcpy(oa->buff+serizlizeLength,&ti.ballot,sizeof(int32_t));
    serizlizeLength+=sizeof(int32_t);
    int32_t status = ti.status;
    memcpy(oa->buff+serizlizeLength,&status,sizeof(int32_t));
    serizlizeLength+=sizeof(int32_t);
    memcpy(oa->buff+serizlizeLength,&ti.seq,sizeof(int32_t));
    serizlizeLength+=sizeof(int32_t);
    for(int i = 0;i < GROUP_SZIE;i++){
        memcpy(oa->buff+serizlizeLength,&ti.deps[i],sizeof(int32_t));
        serizlizeLength+=sizeof(int32_t);
    }
    int cmdSize = ti.cmds.size();
    memcpy(oa->buff+serizlizeLength,&cmdSize,sizeof(int32_t));
    serizlizeLength+=sizeof(int32_t);
    for(int i = 0;i < ti.cmds.size();i++){
        struct tk_command command = ti.cmds.at(i);
        int32_t op = command.opcode;
        memcpy(oa->buff+serizlizeLength,&op,sizeof(int32_t));
        serizlizeLength+=sizeof(int32_t);
        int32_t keySize = command.key.size();
        //cout<<"keySize="<<keySize<<endl;
        memcpy(oa->buff+serizlizeLength,&keySize,sizeof(int32_t));
        serizlizeLength+=sizeof(int32_t);
        char key[keySize] = "";
        for(int j = 0;j < keySize;j++){
            key[j] = command.key[j];
        }
        memcpy(oa->buff+serizlizeLength,key,keySize);
        serizlizeLength+=command.key.size();
        memcpy(oa->buff+serizlizeLength,&command.valSize,sizeof(int32_t));
        serizlizeLength+=sizeof(int32_t);
        memcpy(oa->buff+serizlizeLength,command.val,command.valSize);
        serizlizeLength+=command.valSize;
    }
    //checksum
    unsigned char* checkSumCharArray = (unsigned char*)malloc(length);
    memcpy(checkSumCharArray,oa->buff,length);
    uint32_t checksum = Checksum(checkSumCharArray,length);
    memcpy(oa->buff+serizlizeLength,&checksum,sizeof(uint32_t));
    serizlizeLength+=sizeof(uint32_t);
    char logEnd = 0x42;
    memcpy(oa->buff+serizlizeLength,&logEnd,sizeof(char));
    serizlizeLength+=sizeof(char);
}

