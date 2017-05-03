#include "protobuff.pb.h"
#include <cstring>
#include <string>
#include <iostream>
#include "../../consensus/include/tk_message.h"
#include "../include/tk_jute.h"
//
using namespace std;


void serializePrepare(PreAccept_t *pat,struct serializeBuff* oa){

	tkJute::PrepareJute pa;
	pa.set_ballot(pat->Ballot);
	pa.set_instance(pat->Instance);
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePrepare(struct serializeBuff* ia,struct Prepare *pat){

	tkJute::PrepareJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Ballot = pa.ballot();
        pat->Instance = pa.instance();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializePrepareReply(PrepareReply_t *pat,struct serializeBuff* oa){

	tkJute::PrepareReplyJute pa;
	pa.set_acceptorid(pat->AcceptorId);
	pa.set_ballot(pat->Ballot);
	pa.set_instance(pat->Instance);
	pa.set_ok(pat->Ok);
	pa.set_replica(pat->Replica);
	pa.set_seq(pat->Seq);
	pa.set_status(pat->Status);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
	cmd->set_opcode(pat->cmd.opcode);
	cmd->set_key(pat->cmd.key);
	cmd->set_val(pat->cmd.val);
	pa.set_allocated_cmd(cmd);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePrepareReply(struct serializeBuff* ia, PrepareReply_t *pat){

	tkJute::PrepareReplyJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->AcceptorId = pa.acceptorid();
        pat->Ballot = pa.ballot();
        pat->Instance = pa.instance();
        pat->Ok = pa.ok();
        pat->Seq = pa.seq();
        pat->Replica = pa.replica();
        pat->Status = pa.status();
        struct tk_command cmd;
        cmd.opcode = pa.cmd().opcode();
        cmd.key = (char*)malloc(pa.cmd().key().size());
        cmd.val = (char*)malloc(pa.cmd().val().size());
        strcpy(cmd.key,pa.cmd().key().c_str());
        strcpy(cmd.val,pa.cmd().val().c_str());
        pat->cmd = cmd;
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}


void serializePreAccept(PreAccept_t *pat,struct serializeBuff* oa){

	tkJute::PreAcceptJute pa;
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
	cmd->set_opcode(pat->cmd.opcode);
	cmd->set_key(pat->cmd.key);
	cmd->set_val(pat->cmd.val);
	pa.set_allocated_cmd(cmd);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAccept(struct serializeBuff* ia,struct PreAccept *pat){

	tkJute::PreAcceptJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Ballot = pa.ballot();
        struct tk_command cmd;
        cmd.opcode = pa.cmd().opcode();
        cmd.key = (char*)malloc(pa.cmd().key().size());
        cmd.val = (char*)malloc(pa.cmd().val().size());
        strcpy(cmd.key,pa.cmd().key().c_str());
        strcpy(cmd.val,pa.cmd().val().c_str());
        pat->cmd = cmd;
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
        pat->Instance = pa.instance();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Seq = pa.seq();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializePreAcceptReply(PreAcceptReply_t *pat,struct serializeBuff* oa){

	tkJute::PreAcceptReplyJute pa;
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ok(pat->Ok);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);

	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	for(int i = 0;i < 5;i++){
        pa.add_committeddeps(pat->CommittedDeps[i]);
	}
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAcceptReply(struct serializeBuff* ia,struct PreAcceptReply *pat){

	tkJute::PreAcceptReplyJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ok = pa.ok();
        pat->Ballot = pa.ballot();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
        for(int i = 0;i < pa.committeddeps_size();i++){
            pat->CommittedDeps[i] = pa.committeddeps(i);
        }

	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializePreAcceptOk(PreAcceptOk_t *pat,struct serializeBuff* oa){

	tkJute::PreAcceptOkJute pa;
	pa.set_instance(pat->Instance);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAcceptOk(struct serializeBuff* ia,struct PreAcceptOk *pat){

	tkJute::PreAcceptOkJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Instance = pa.instance();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializeAccept(Accept_t *pat,struct serializeBuff* oa){

	tkJute::AcceptJute pa;
	pa.set_leaderid(pat->LeaderId);
    pa.set_replica(pat->Replica);
    pa.set_instance(pat->Instance);
    pa.set_ballot(pat->Ballot);
    pa.set_count(pat->Count);
    pa.set_seq(pat->Seq);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeAccept(struct serializeBuff* ia,struct Accept *pat){

	tkJute::AcceptJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Count = pa.count();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }

	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializeAcceptReply(AcceptReply_t *pat,struct serializeBuff* oa){

	tkJute::AcceptReplyJute pa;
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ok(pat->Ok);
	pa.set_ballot(pat->Ballot);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeAcceptReply(struct serializeBuff* ia,struct AcceptReply *pat){

	tkJute::AcceptReplyJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ok = pa.ok();
        pat->Ballot = pa.ballot();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializeCommit(Commit_t *pat,struct serializeBuff* oa){

	tkJute::CommitJute pa;
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
	cmd->set_opcode(pat->cmd.opcode);
	cmd->set_key(pat->cmd.key);
	cmd->set_val(pat->cmd.val);
	pa.set_allocated_cmd(cmd);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeCommit(struct serializeBuff* ia,struct Commit *pat){

	tkJute::CommitJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        struct tk_command cmd;
        cmd.opcode = pa.cmd().opcode();
        cmd.key = (char*)malloc(pa.cmd().key().size());
        cmd.val = (char*)malloc(pa.cmd().val().size());
        strcpy(cmd.key,pa.cmd().key().c_str());
        strcpy(cmd.val,pa.cmd().val().c_str());
        pat->cmd = cmd;
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
        pat->Instance = pa.instance();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Seq = pa.seq();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();
}

void serializeCommitShort(CommitShort_t *pat,struct serializeBuff* oa){

	tkJute::CommitShortJute pa;
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_count(pat->Count);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeCommitShort(struct serializeBuff* ia,struct CommitShort *pat){

	tkJute::CommitShortJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
        pat->Instance = pa.instance();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Count = pa.count();
        pat->Seq = pa.seq();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();
}

void serializeTryPreAccept(struct TryPreAccept *pat,struct serializeBuff* oa){

	tkJute::TryPreAcceptJute pa;
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < 5;i++){
        pa.add_deps(pat->Deps[i]);
	}
	tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
	cmd->set_opcode(pat->cmd.opcode);
	cmd->set_key(pat->cmd.key);
	cmd->set_val(pat->cmd.val);
	pa.set_allocated_cmd(cmd);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeTryPreAccept(struct serializeBuff* ia,struct TryPreAccept *pat){

	tkJute::TryPreAcceptJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->Ballot = pa.ballot();
        struct tk_command cmd;
        cmd.opcode = pa.cmd().opcode();
        cmd.key = (char*)malloc(pa.cmd().key().size());
        cmd.val = (char*)malloc(pa.cmd().val().size());
        strcpy(cmd.key,pa.cmd().key().c_str());
        strcpy(cmd.val,pa.cmd().val().c_str());
        pat->cmd = cmd;
        for(int i = 0;i < pa.deps_size();i++){
            pat->Deps[i] = pa.deps(i);
        }
        pat->Instance = pa.instance();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Seq = pa.seq();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}

void serializeTryPreAcceptReply(TryPreAcceptReply_t *pat,struct serializeBuff* oa){

	tkJute::TryPreAcceptReplyJute pa;
	pa.set_acceptorid(pat->AcceptorId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ok(pat->Ok);
	pa.set_ballot(pat->Ballot);
	pa.set_confilctreplica(pat->ConfilctReplica);
	pa.set_confilctinstance(pat->ConfilctInstance);
	pa.set_confilctstatus(pat->ConfilctStatus);
	int64_t len = pa.ByteSize();
	char buff[len] = "";
	bool f = pa.SerializeToArray(&buff,len);
	oa->buff = (char*)malloc(len);
	memcpy(oa->buff,buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeTryPreAcceptReply(struct serializeBuff* ia,struct TryPreAcceptReply *pat){

	tkJute::TryPreAcceptReplyJute pa;
	int32_t len = ia->len;
	char buff[len] = "";
	memcpy(buff,ia->buff,len);
	bool f = pa.ParseFromArray(buff,len);
	if(f){
        pat->AcceptorId = pa.acceptorid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ok = pa.ok();
        pat->Ballot = pa.ballot();
        pat->ConfilctReplica = pa.confilctreplica();
        pat->ConfilctInstance = pa.confilctinstance();
        pat->ConfilctStatus = pa.confilctstatus();
	}else{
        cout<<"deserilizePreAccept fail"<<endl;
	}

	google::protobuf::ShutdownProtobufLibrary();

}


