#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include "include/tk_txn.h"
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
    testStoreInstance();
}



