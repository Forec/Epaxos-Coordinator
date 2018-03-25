#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include "../include/tk_txn.h"
using namespace std;

/*
    存入文件
    append(int32_t replicaId,struct tk_instance ti)

    读取instance,结果保存在result
    struct readBuff rb = readLogFromFile(1);
    struct tk_instance result;
    while(nextInstance(&result,&rb)==1){
        //todo
    }
*/

void testStoreDataTree(){
    //init data tree
    dataTree *dt = new dataTree();
    dt->root.path = "/";
    dataNode * newNode = new dataNode;
    struct buffer buff;
    string testdata = "this is a test node";
    buff.len = testdata.size();
    buff.buff = (char*)malloc(buff.len);
    memcpy(buff.buff,testdata.c_str(),buff.len);
    newNode->parentNode = &dt->root;
    newNode->stat.ctime = 12345;
    newNode->stat.mtime = 12345;
    newNode->stat.czxid = 100;
    newNode->stat.mzxid = 100;
    newNode->stat.pzxid = 100;
    newNode->stat.aversion = 0;
    newNode->stat.cversion = 0;
    newNode->stat.ephemeralOwner = 0;
    newNode->data = buff;
    newNode->path = "/testnode";
    //node /testnode/secondnode
    dataNode * secondNode = new dataNode;
    struct buffer secondNodeBuff;
    string test2data = "this is a second node";
    secondNodeBuff.len = test2data.size();
    secondNodeBuff.buff = (char*)malloc(secondNodeBuff.len);
    memcpy(secondNodeBuff.buff,test2data.c_str(),secondNodeBuff.len);
    secondNode->parentNode = newNode;
    secondNode->stat.ctime = 23456;
    secondNode->stat.mtime = 12345;
    secondNode->stat.czxid = 100;
    secondNode->stat.mzxid = 100;
    secondNode->stat.pzxid = 100;
    secondNode->stat.aversion = 0;
    secondNode->stat.cversion = 0;
    secondNode->stat.ephemeralOwner = 0;
    secondNode->data = buff;
    secondNode->path = "/testnode/secondNode";
    //
    newNode->son.insert(pair<string,dataNode*>("secondNode",secondNode));
    dt->root.son.insert(pair<string,dataNode*>("testnode",newNode));
    map<std::string,struct dataNode_t *>::iterator iter;
    for(iter=dt->root.son.begin();iter!=dt->root.son.end();iter++ ){
        cout<<iter->second->stat.ctime<<endl;
    }
    cout<<dt->root.son.at("testnode")->son.size()<<endl;
    cout<<"start writing datanode to file"<<endl;
    //write data tree to file
    writeDataTreeToFile(dt);
    //read data tree to file
    cout<<"start read datanode from file"<<endl;
    dataTree *result = new dataTree();
    string fileName = "data";
    int fd = open(fileName.c_str(),O_RDONLY);
    if(fd < 0){
        cout<<"there is no data file"<<endl;
    }else{
        readDataTreeFromFile(result,fileName,fd);
    }
    cout<<result->root.son.size()<<endl;
    cout<<result->root.son.at("testnode")->son.at("secondNode")->stat.ctime<<endl;
    string resultData(result->root.son.at("testnode")->data.buff);
    cout<<resultData<<endl;
}

void testStoreInstance(){

    //init instance
    string com[] = {"c1","c2","c3","c4","c5","c6"};
    char val[] = "value";
    vector<vector<tk_command> > command;
    array<array<int32_t,GROUP_SZIE>,6> dep;
    for(int i = 0;i < 6;i++){
        vector<tk_command> vtc;
        struct tk_command tc;
        OP op = GET;
        tc.opcode = op;
        tc.key = com[i];
        tc.valSize = 6;
        tc.val = val;
        vtc.push_back(tc);
        command.push_back(vtc);
        array<int32_t,GROUP_SZIE> depTmp;
        for(int i = 0;i < GROUP_SZIE;i++){
            depTmp[i] = i;
        }
        dep[i] = depTmp;
    }
    for(int i = 0;i < 6;i++){
        struct tk_instance ti;
        ti.ballot = i;
        ti.status = (enum STATUS)i;
        ti.seq = i;
        ti.deps = dep.at(i);
        ti.cmds = command.at(i);
        append(1,ti);
    }

    //readFromFile
    struct readBuff rb = readInstanceFromFile(1);
    struct tk_instance result;
    int j = 1;
    while(nextInstance(&result,&rb)==1){
        cout<<"instance"<<j<<endl;
        cout<<"ballot="<<result.ballot<<endl;
        cout<<"status="<<result.status<<endl;
        cout<<"seq="<<result.seq<<endl;
        cout<<"deps=";
        for(int i = 0;i < GROUP_SZIE;i++){
            cout<<" "<<result.deps.at(i);
        }
        cout<<endl;
        for(int i = 0;i < result.cmds.size();i++){
            cout<<"command"<<i<<endl;
            cout<<"op="<<result.cmds.at(i).opcode<<endl;
            cout<<"key="<<result.cmds.at(i).key<<endl;
            cout<<"val="<<result.cmds.at(i).val<<endl;
        }
        j++;
    }
}

int main(){
    //testStoreInstance();
    testStoreDataTree();
}



