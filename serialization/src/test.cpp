//
// Created by haboa on 17-12-10.
//
#include "../include/tk_jute.h"
#include <iostream>
#include <gtest/gtest.h>

using namespace std;

struct tk_command getTkCommand(){
    struct tk_command command;
    command.opcode = CREATESESSION;
    command.key = "testkey";
    command.owner = 10;
    command.sessionId = 201204045;
    string val = "testval";
    int32_t size = val.size();
    command.valSize = size;
    command.val = (char *)malloc(size);
    memcpy(command.val,val.c_str(),size);
    return command;
}

void eqTkCommand(struct tk_command a, struct tk_command b){
    ASSERT_EQ(a.opcode,b.opcode);
    ASSERT_EQ(a.key,b.key);
    ASSERT_EQ(a.sessionId,b.sessionId);
    ASSERT_EQ(a.owner,b.owner);
    ASSERT_EQ(a.valSize,b.valSize);
    ASSERT_STREQ(a.val,b.val);
}

TEST(protobuff__Test,tkCommand){
    struct tk_command command = getTkCommand();
    struct serializeBuff sb;
    serializeTkCommand(&command,&sb);
    struct tk_command result;
    deserializeTkCommand(&sb,&result);
    eqTkCommand(command,result);
}

TEST(protobuff__Test,Prepare){
    struct Prepare pat;
    pat.Type = READ;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    pat.LeaderId = 5;
    struct serializeBuff sb;
    serializePrepare(&pat,&sb);
    struct Prepare result;
    deserializePrepare(&sb,&result);
    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
}

TEST(protobuff__Test,PrepareReply){
    struct PrepareReply pat;
    pat.Type = READ;
    pat.Ok = true;
    pat.Status = ACCEPTED;
    pat.AcceptorId = 1;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    pat.Seq = 5;
    struct tk_command command = getTkCommand();
    for(int i = 0;i < GROUP_SIZE;i++){
        command.sessionId = i;
        pat.Command.push_back(command);
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializePrepareReply(&pat,&sb);
    struct PrepareReply result;
    deserializePrepareReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ok,pat.Ok);
    ASSERT_EQ(result.Status,pat.Status);
    ASSERT_EQ(result.AcceptorId,pat.AcceptorId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        eqTkCommand(result.Command.at(i),pat.Command.at(i));
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,PreAccept){
    struct PreAccept pat;
    pat.Type = READ;
    pat.LeaderId = 5;
    pat.Replica = 2;
    pat.Instance = 4;
    pat.Ballot = 3;
    pat.Seq = 6;
    struct tk_command command = getTkCommand();
    for(int i = 0;i < 7;i++){
        command.sessionId = i;
        pat.Command.push_back(command);
    }
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializePreAccept(&pat,&sb);
    struct PreAccept result;
    deserializePreAccept(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < 3;i++){
        eqTkCommand(result.Command.at(i),pat.Command.at(i));
    }
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,PreAcceptReply){
    struct PreAcceptReply pat;
    pat.Type = READ;
    pat.Ok = false;
    pat.Replica = 2;
    pat.Instance = 4;
    pat.Ballot = 3;
    pat.Seq = 6;
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.Deps[i] = i;
        pat.CommittedDeps[i] = i+100;
    }
    struct serializeBuff sb;
    serializePreAcceptReply(&pat,&sb);
    struct PreAcceptReply result;
    deserializePreAcceptReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ok,pat.Ok);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
        ASSERT_EQ(result.CommittedDeps.at(i),pat.CommittedDeps.at(i));
    }
}

TEST(protobuff__Test,PreAcceptOk){
    struct PreAcceptOk pat;
    pat.Type = READ;
    pat.Instance = 100;
    struct serializeBuff sb;
    serializePreAcceptOk(&pat,&sb);
    struct PreAcceptOk result;
    deserializePreAcceptOk(&sb,&result);
    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Instance,pat.Instance);
}

TEST(protobuff__Test,Accpet){
    struct Accept pat;
    pat.Type = READ;
    pat.LeaderId = 1;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    pat.Count = 6;
    pat.Seq = 5;
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializeAccept(&pat,&sb);
    struct Accept result;
    deserializeAccept(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Count,pat.Count);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,AccpetReply){
    struct AcceptReply pat;
    pat.Type = READ;
    pat.Ok = true;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    struct serializeBuff sb;
    serializeAcceptReply(&pat,&sb);
    struct AcceptReply result;
    deserializeAcceptReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ok,pat.Ok);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
}

TEST(protobuff__Test,Commit){
    struct Commit pat;
    pat.Type = READ;
    pat.LeaderId = 1;
    pat.Replica = 2;
    pat.Instance = 4;
    pat.Seq = 5;
    struct tk_command command = getTkCommand();
    for(int i = 0;i < GROUP_SIZE;i++){
        command.sessionId = i;
        pat.Command.push_back(command);
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializeCommit(&pat,&sb);
    struct Commit result;
    deserializeCommit(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        eqTkCommand(result.Command.at(i),pat.Command.at(i));
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,CommitShort){
    struct CommitShort pat;
    pat.Type = READ;
    pat.LeaderId = 1;
    pat.Replica = 2;
    pat.Instance = 4;
    pat.Count = 4;
    pat.Seq = 5;
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializeCommitShort(&pat,&sb);
    struct CommitShort result;
    deserializeCommitShort(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Count,pat.Count);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,TryPreAccept){
    struct TryPreAccept pat;
    pat.Type = READ;
    pat.LeaderId = 1;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    pat.Seq = 5;
    struct tk_command command = getTkCommand();
    for(int i = 0;i < GROUP_SIZE;i++){
        command.sessionId = i;
        pat.Command.push_back(command);
        pat.Deps[i] = i;
    }
    struct serializeBuff sb;
    serializeTryPreAccept(&pat,&sb);
    struct TryPreAccept result;
    deserializeTryPreAccept(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.Seq,pat.Seq);
    for(int i = 0;i < GROUP_SIZE;i++){
        eqTkCommand(result.Command.at(i),pat.Command.at(i));
        ASSERT_EQ(result.Deps.at(i),pat.Deps.at(i));
    }
}

TEST(protobuff__Test,TryPreAcceptReply){
    struct TryPreAcceptReply pat;
    pat.Type = READ;
    pat.Ok = true;
    pat.AcceptorId = 1;
    pat.Replica = 2;
    pat.Ballot = 3;
    pat.Instance = 4;
    pat.ConflictReplica = 5;
    pat.ConflictInstance = 5;
    pat.ConflictStatus = 5;

    struct serializeBuff sb;
    serializeTryPreAcceptReply(&pat,&sb);
    struct TryPreAcceptReply result;
    deserializeTryPreAcceptReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ok,pat.Ok);
    ASSERT_EQ(result.AcceptorId,pat.AcceptorId);
    ASSERT_EQ(result.Replica,pat.Replica);
    ASSERT_EQ(result.Ballot,pat.Ballot);
    ASSERT_EQ(result.Instance,pat.Instance);
    ASSERT_EQ(result.ConflictReplica,pat.ConflictReplica);
    ASSERT_EQ(result.ConflictInstance,pat.ConflictInstance);
    ASSERT_EQ(result.ConflictStatus,pat.ConflictStatus);
}

TEST(protobuff__Test,Propose){
    struct Propose pat;
    pat.Type = READ;
    pat.CommandId = 1;
    pat.Timestamp = 2;
    pat.Command = getTkCommand();
    struct serializeBuff sb;
    serializePropose(&pat,&sb);
    struct Propose result;
    deserializePropose(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.CommandId,pat.CommandId);
    ASSERT_EQ(result.Timestamp,pat.Timestamp);
    eqTkCommand(result.Command,pat.Command);
}

TEST(protobuff__Test,ProposeReply){
    struct ProposeReply pat;
    pat.Type = READ;
    pat.OK = true;
    pat.CommandId = 100;
    struct serializeBuff sb;
    serializeProposeReply(&pat,&sb);
    struct ProposeReply result;
    deserializeProposeReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.OK,pat.OK);
    ASSERT_EQ(result.CommandId,pat.CommandId);
}

TEST(protobuff__Test,ProposeReplyTS){
    struct ProposeReplyTS pat;
    pat.Type = READ;
    pat.OK = true;
    pat.CommandId = 100;
    pat.ValueSize = 10;
    pat.Timestamp = 20;
    pat.Value = "testval";
    struct serializeBuff sb;
    serializeProposeReplyTS(&pat,&sb);
    struct ProposeReplyTS result;
    deserializeProposeReplyTS(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.OK,pat.OK);
    ASSERT_EQ(result.CommandId,pat.CommandId);
    ASSERT_EQ(result.ValueSize,pat.ValueSize);
    ASSERT_EQ(result.Timestamp,pat.Timestamp);
    ASSERT_STREQ(result.Value,pat.Value);
}

TEST(protobuff__Test,Read){
    struct Read pat;
    pat.Type = READ;
    pat.CommandId = 100;
    pat.Key = "testkey";
    struct serializeBuff sb;
    serializeRead(&pat,&sb);
    struct Read result;
    deserializeRead(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.CommandId,pat.CommandId);
    ASSERT_STREQ(result.Key.c_str(),pat.Key.c_str());
}

TEST(protobuff__Test,ProposeAndRead){
    struct ProposeAndRead pat;
    pat.Type = READ;
    pat.CommandId = 100;
    pat.Command = getTkCommand();
    pat.Key = "testkey";
    struct serializeBuff sb;
    serializeProposeAndRead(&pat,&sb);
    struct ProposeAndRead result;
    deserializeProposeAndRead(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.CommandId,pat.CommandId);
    ASSERT_STREQ(result.Key.c_str(),pat.Key.c_str());
    eqTkCommand(result.Command,pat.Command);
}

TEST(protobuff__Test,InstanceId){
    struct InstanceId pat;
    pat.Type = READ;
    pat.replica = 1;
    pat.instance = 2;
    struct serializeBuff sb;
    serializeInstanceId(&pat,&sb);
    struct InstanceId result;
    deserializeInstanceId(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.replica,pat.replica);
    ASSERT_EQ(result.instance,pat.instance);
}

TEST(protobuff__Test,Clock){
    struct Clock pat;
    pat.Type = READ;
    struct serializeBuff sb;
    serializeClock(&pat,&sb);
    struct Clock result;
    deserializeClock(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
}

TEST(protobuff__Test,Beacon_msg){
    struct Beacon_msg pat;
    pat.Type = READ;
    pat.Rid = 1;
    pat.timestamp = 100;
    struct serializeBuff sb;
    serializeBeacon_msg(&pat,&sb);
    struct Beacon_msg result;
    deserializeBeacon_msg(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Rid,pat.Rid);
    ASSERT_EQ(result.timestamp,pat.timestamp);
}

TEST(protobuff__Test,Beacon_msg_reply){
    struct Beacon_msg_reply pat;
    pat.Type = READ;
    pat.timestamp = 100;
    struct serializeBuff sb;
    serializeBeacon_msg_reply(&pat,&sb);
    struct Beacon_msg_reply result;
    deserializeBeacon_msg_reply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.timestamp,pat.timestamp);
}

TEST(protobuff__Test,ClientConnect){
    struct ClientConnect pat;
    pat.Type = READ;
    struct serializeBuff sb;
    serializeClientConnect(&pat,&sb);
    struct ClientConnect result;
    deserializeClientConnect(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
}

TEST(protobuff__Test,RegisterArgs){
    struct RegisterArgs pat;
    pat.Type = READ;
    pat.Addr = "testaddr";
    pat.Port = 1;
    struct serializeBuff sb;
    serializeRegisterArgs(&pat,&sb);
    struct RegisterArgs result;
    deserializeRegisterArgs(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_STREQ(result.Addr.c_str(),pat.Addr.c_str());
    ASSERT_EQ(result.Port,pat.Port);
}

TEST(protobuff__Test,RegisterReply){
    struct RegisterReply pat;
    pat.Type = READ;
    pat.ReplicaId = 10;
    pat.Ready = true;
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.AddrList.push_back("testaddr");
        pat.PortList.push_back(i+100);
    }
    struct serializeBuff sb;
    serializeRegisterReply(&pat,&sb);
    struct RegisterReply result;
    deserializeRegisterReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.ReplicaId,pat.ReplicaId);
    ASSERT_EQ(result.Ready,pat.Ready);
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_STREQ(result.AddrList.at(i).c_str(),pat.AddrList.at(i).c_str());
        ASSERT_EQ(result.PortList[i],pat.PortList[i]);
    }
}

TEST(protobuff__Test,GetLeaderArgs){
    struct GetLeaderArgs pat;
    pat.Type = READ;
    struct serializeBuff sb;
    serializeGetLeaderArgs(&pat,&sb);
    struct GetLeaderArgs result;
    deserializeGetLeaderArgs(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
}

TEST(protobuff__Test,GetLeaderReply){
    struct GetLeaderReply pat;
    pat.Type = READ;
    pat.LeaderId = 10;
    struct serializeBuff sb;
    serializeGetLeaderReply(&pat,&sb);
    struct GetLeaderReply result;
    deserializeGetLeaderReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.LeaderId,pat.LeaderId);
}

TEST(protobuff__Test,GetReplicaListArgs){
    struct GetReplicaListArgs pat;
    pat.Type = READ;
    struct serializeBuff sb;
    serializeGetReplicaListArgs(&pat,&sb);
    struct GetReplicaListArgs result;
    deserializeGetReplicaListArgs(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
}

TEST(protobuff__Test,GetReplicaListReply){
    struct GetReplicaListReply pat;
    pat.Type = READ;
    pat.Ready = true;
    for(int i = 0;i < GROUP_SIZE;i++){
        pat.ReplicaAddrList.push_back("testaddr");
        pat.ReplicaPortList.push_back(i+100);
    }
    struct serializeBuff sb;
    serializeGetReplicaListReply(&pat,&sb);
    struct GetReplicaListReply result;
    deserializeGetReplicaListReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ready,pat.Ready);
    for(int i = 0;i < GROUP_SIZE;i++){
        ASSERT_STREQ(result.ReplicaAddrList.at(i).c_str(),pat.ReplicaAddrList.at(i).c_str());
        ASSERT_EQ(result.ReplicaPortList[i],pat.ReplicaPortList[i]);
    }
}

TEST(protobuff__Test,BeTheLeaderReply){
    struct BeTheLeaderReply pat;
    pat.Type = READ;
    pat.Ok = true;
    struct serializeBuff sb;
    serializeBeTheLeaderReply(&pat,&sb);
    struct BeTheLeaderReply result;
    deserializeBeTheLeaderReply(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
    ASSERT_EQ(result.Ok,pat.Ok);
}

TEST(protobuff__Test,GENERAL){
    struct GENERAL pat;
    pat.Type = READ;
    struct serializeBuff sb;
    serializeGENERAL(&pat,&sb);
    struct GENERAL result;
    deserializeGENERAL(&sb,&result);

    ASSERT_EQ(result.Type,pat.Type);
}

int main(int argc, char **argv){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
