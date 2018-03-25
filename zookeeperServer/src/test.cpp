//
// Created by haboa on 17-10-18.
//


#include <cstring>
#include <string>
#include <iostream>
#include "../include/zookeeperServer.jute.h"
#include "../include/zookeeperServerSnapshot.h"
using namespace std;


void serializeTest(){
    //测试　例子使用ＰｒｅＡｃｃｅｐｔ
    /*序列结果存储在serilizeBuff中,结果为ｖｏｉｄ*,长度为ｌｅｎ
    	struct serializeBuff oa;
    	serializePreAccept(&pat,&oa);
    */
    /*反序列化
		PreAccept_t result;
    	deserializePreAccept(&ia,&result);
    */
    PreAccept_t pat;
    pat.Ballot = 1;
    pat.Instance = 2;
    pat.LeaderId = 4;
    pat.Replica = 5;
    pat.Seq = 6;
    for(int i = 0;i<5;i++){
        pat.Deps[i] = i;
    }
    tk_command_t tc;
    tc.key ="key";
    tc.opcode = '1';
    tc.val = "val";
    pat.cmd = tc;
    struct serializeBuff oa;
    serializePreAccept(&pat,&oa);
    struct serializeBuff ia;
    ia.len = oa.len;
    ia.buff = (char*)malloc(oa.len);
    memcpy(ia.buff,oa.buff,oa.len);
    PreAccept_t result;
    deserializePreAccept(&ia,&result);
    cout<<"result"<<endl;
    cout<<"Ballot="<<result.Ballot<<endl;
    cout<<"Instance="<<result.Instance<<endl;
    cout<<"LeaderId="<<result.LeaderId<<endl;
    cout<<"Replica="<<result.Replica<<endl;
    cout<<"cmd opcode="<<result.cmd.opcode<<endl;
    cout<<"cmd key="<<result.cmd.key<<endl;
    cout<<"cmd val="<<result.cmd.val<<endl;
}

void testTxn(){
    //写进去６个ｌｏｇ，写入不同的文件，然后从中读取出来，文件名以存储的第一个ｌｏｇ的ｚｘｉｄ存储
    //第一个文件存２个ｌｏｇ，第二个文件存３个ｌｏｇ，第三个ｌｏｇ存１个ｌｏｇ
    //写ｌｏｇ
    /*
        FileTxnLog_t FTL1;
        initFileTxnLog(&FTL1,1);
        append(&FTL1,&txn);
    */
    /*读ｌｏｇ
        int logNumber = 1;//从logNumber读取log，读出该number的log或该numberd 下一个log
        struct readBuff rb = readLogFromFile(logNumber);
        Txn_t result;
        while(nextLog(&result,1,&rb)==1){//1表示读取成功，２表示校验错误，３表示读取结束

        }
    */
    FileTxnLog_t FTL1;
    FileTxnLog_t FTL2;
    FileTxnLog_t FTL3;
    initFileTxnLog(&FTL1,1);
    initFileTxnLog(&FTL2,1);
    initFileTxnLog(&FTL3,1);
    TxnHeader_t txnHeader1 = {1,1,1,1,1,"status1"};
    TxnHeader_t txnHeader2 = {2,2,2,2,2,"status2"};
    TxnHeader_t txnHeader3 = {3,3,3,3,3,"status3"};
    TxnHeader_t txnHeader4 = {4,4,4,4,4,"status4"};
    TxnHeader_t txnHeader5 = {5,5,5,5,5,"status5"};
    TxnHeader_t txnHeader6 = {6,6,6,6,6,"status6"};
    char log1[] = "log1";
    char log2[] = "log2";
    char log3[] = "log3";
    char log4[] = "log4";
    char log5[] = "log5";
    char log6[] = "log6";
    //  char* result = (char*)malloc(4);
    struct serializeBuff log;
    log.len = 4;
    log.buff  = (char*)malloc(4);

    Txn_t txn;
    //log1
    memcpy(log.buff,log1,4);
    txn.hdr = txnHeader1;
    txn.log = log;
    txn.recordtype = 1;
    if(!append(&FTL1,&txn)){
        cout<<"创建文件失败"<<endl;
    }
    //log2
    memcpy(log.buff,log2,4);
    txn.hdr = txnHeader2;
    txn.log = log;
    txn.recordtype = 1;
    append(&FTL1,&txn);
    //log3
    memcpy(log.buff,log3,4);
    txn.hdr = txnHeader3;
    txn.log = log;
    txn.recordtype = 1;
    append(&FTL2,&txn);
    //log4
    memcpy(log.buff,log4,4);
    txn.hdr = txnHeader4;
    txn.log = log;
    txn.recordtype = 1;
    append(&FTL2,&txn);
    //log5
    memcpy(log.buff,log5,4);
    txn.hdr = txnHeader5;
    txn.log = log;
    txn.recordtype = 1;
    append(&FTL2,&txn);
    //log6
    memcpy(log.buff,log6,4);
    txn.hdr = txnHeader6;
    txn.log = log;
    txn.recordtype = 1;
    append(&FTL3,&txn);
    //readFromFile
    struct readBuff rb = readLogFromFile(1);
    Txn_t result;
    int i = 1;//只是为了打印
    //从第一个ｌｏｇ读取
    while(nextLog(&result,1,&rb)==1){
        cout<<"read "<<i<<"log done"<<endl;
        cout<<"log"<<i<<"sessonid="<<result.hdr.sessionId<<endl;
        cout<<"log"<<i<<"cxid="<<result.hdr.cxid<<endl;
        cout<<"log"<<i<<"zxid="<<result.hdr.zxid<<endl;
        cout<<"log"<<i<<"time="<<result.hdr.time<<endl;
        cout<<"log"<<i<<"type="<<result.hdr.cxid<<endl;
        cout<<"log"<<i<<"status="<<result.hdr.status<<endl;
        char log[result.log.len+1];
        memcpy(log,result.log.buff,result.log.len);
        log[result.log.len] = '\0';
        string str = log;
        cout<<"log"<<i<<"log="<<str<<endl;
        cout<<"log"<<i<<"recordtype="<<result.recordtype<<endl;
        i++;
    }
    //从第４个ｌｏｇ读取
    //readFromFile
    rb = readLogFromFile(4);
    i = 4;
    while(nextLog(&result,4,&rb)==1){
        cout<<"read "<<i<<"log done"<<endl;
        cout<<"log"<<i<<"sessonid="<<result.hdr.sessionId<<endl;
        cout<<"log"<<i<<"cxid="<<result.hdr.cxid<<endl;
        cout<<"log"<<i<<"zxid="<<result.hdr.zxid<<endl;
        cout<<"log"<<i<<"time="<<result.hdr.time<<endl;
        cout<<"log"<<i<<"type="<<result.hdr.cxid<<endl;
        cout<<"log"<<i<<"status="<<result.hdr.status<<endl;
        char log[result.log.len+1];
        memcpy(log,result.log.buff,result.log.len);
        log[result.log.len] = '\0';
        string str = log;
        cout<<"log"<<i<<"log="<<str<<endl;
        cout<<"log"<<i<<"recordtype="<<result.recordtype<<endl;
        i++;
    }
}

int main(){
    serializeTest();
    //testTxn();
}



