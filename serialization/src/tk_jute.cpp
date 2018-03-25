#include <cstring>
#include <string>
#include <iostream>
#include "../include/tk_jute.h"
using namespace std;

void serializeTkCommand(struct tk_command * command,serializeBuff * oa){
    tkJute::tk_command_jute commandJute;
    commandJute.set_opcode(command->opcode);
    commandJute.set_key(command->key);
    commandJute.set_sessioid(command->sessionId);
    commandJute.set_owner(command->owner);
    commandJute.set_valsize(command->valSize);
    commandJute.set_val(command->val);
    int64_t len = commandJute.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = commandJute.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}

void deserializeTkCommand(struct serializeBuff * ia,struct tk_command * command){
    tkJute::tk_command_jute commandJute;
    bool f = commandJute.ParseFromArray(ia->buff,ia->len);
    if(f){
        command->opcode = (OP)commandJute.opcode();
        command->key = commandJute.key();
        command->sessionId = commandJute.sessioid();
        command->owner = commandJute.owner();
        int valSize = commandJute.valsize();
        command->valSize = valSize;
        command->val = (char*)malloc(valSize);
        memcpy(command->val,commandJute.val().c_str(),valSize);
    }else{
        cout<<"deserialize tk command fail"<<endl;
    }

    google::protobuf::ShutdownProtobufLibrary();
}

void serializePrepare(struct Prepare *pat,struct serializeBuff* oa){

	tkJute::PrepareJute pa;
    pa.set_type(pat->Type);
    pa.set_leaderid(pat->LeaderId);
    pa.set_replica(pat->Replica);
    pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePrepare(struct serializeBuff* ia,struct Prepare *pat){
	tkJute::PrepareJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
	}else{
        cout<<"deserialize prepare fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();
}

void serializePrepareReply(struct PrepareReply *pat,struct serializeBuff* oa){
	tkJute::PrepareReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->Ok);
    pa.set_status(pat->Status);
	pa.set_acceptorid(pat->AcceptorId);
    pa.set_replica(pat->Replica);
    pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
    for(int i = 0;i < pat->Command.size();i++){
        tkJute::tk_command_jute* cmd = pa.add_command();
        cmd->set_opcode(pat->Command.at(i).opcode);
        cmd->set_key(pat->Command.at(i).key);
        cmd->set_sessioid(pat->Command.at(i).sessionId);
        cmd->set_owner(pat->Command.at(i).owner);
        cmd->set_valsize(pat->Command.at(i).valSize);
        cmd->set_val(pat->Command.at(i).val);
    }
	for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
    }
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();
}

void deserializePrepareReply(struct serializeBuff* ia,struct PrepareReply *pat){
	tkJute::PrepareReplyJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ok = pa.ok();
        pat->Status = (STATUS)pa.status();
        pat->AcceptorId = pa.acceptorid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.command_size();i++){
            struct tk_command cmd;
            cmd.opcode = (OP)pa.command(i).opcode();
            cmd.key = pa.command(i).key();
            cmd.sessionId = pa.command(i).sessioid();
            cmd.owner = pa.command(i).owner();
            cmd.valSize = pa.command(i).valsize();
            cmd.val = (char *)malloc(cmd.valSize);
            memcpy(cmd.val,pa.command(i).val().c_str(),cmd.valSize);
            pat->Command.push_back(cmd);
        }
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize PrepareReply fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();
}


void serializePreAccept(struct PreAccept *pat,struct serializeBuff* oa){

	tkJute::PreAcceptJute pa;
    pa.set_type(pat->Type);
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
    for(int i = 0;i < pat->Command.size();i++){
        tkJute::tk_command_jute* cmd = pa.add_command();
        cmd->set_opcode(pat->Command.at(i).opcode);
        cmd->set_key(pat->Command.at(i).key);
        cmd->set_sessioid(pat->Command.at(i).sessionId);
        cmd->set_owner(pat->Command.at(i).owner);
        cmd->set_valsize(pat->Command.at(i).valSize);
        cmd->set_val(pat->Command.at(i).val);
    }
    for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
    }
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAccept(struct serializeBuff* ia,struct PreAccept *pat){

	tkJute::PreAcceptJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.command_size();i++){
            struct tk_command cmd;
            cmd.opcode = (OP)pa.command(i).opcode();
            cmd.key = pa.command(i).key();
            cmd.sessionId = pa.command(i).sessioid();
            cmd.owner = pa.command(i).owner();
            cmd.valSize = pa.command(i).valsize();
            cmd.val = (char *)malloc(cmd.valSize);
            memcpy(cmd.val,pa.command(i).val().c_str(),cmd.valSize);
            pat->Command.push_back(cmd);
        }
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize PreAccept fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

void serializePreAcceptReply(struct PreAcceptReply *pat,struct serializeBuff* oa){

	tkJute::PreAcceptReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->Ok);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
        pa.add_committeddeps(pat->CommittedDeps[i]);
	}
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAcceptReply(struct serializeBuff* ia,struct PreAcceptReply *pat){

	tkJute::PreAcceptReplyJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ok = pa.ok();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Seq = pa.seq();
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
            pat->CommittedDeps[i] = pa.committeddeps(i);
        }
	}else{
        cout<<"deserialize PreAccept reply fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();
}

void serializePreAcceptOk(struct PreAcceptOk *pat,struct serializeBuff* oa){

	tkJute::PreAcceptOkJute pa;
	pa.set_instance(pat->Instance);
    pa.set_type(pat->Type);
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializePreAcceptOk(struct serializeBuff* ia,struct PreAcceptOk *pat){

	tkJute::PreAcceptOkJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->Instance = pa.instance();
	}else{
        cout<<"deserialize PreAcceptOk fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

void serializeAccept(struct Accept *pat,struct serializeBuff* oa){

	tkJute::AcceptJute pa;
    pa.set_type(pat->Type);
	pa.set_leaderid(pat->LeaderId);
    pa.set_replica(pat->Replica);
    pa.set_instance(pat->Instance);
    pa.set_ballot(pat->Ballot);
    pa.set_count(pat->Count);
    pa.set_seq(pat->Seq);
	for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
	}
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeAccept(struct serializeBuff* ia,struct Accept *pat){

	tkJute::AcceptJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Count = pa.count();
        pat->Seq = pa.seq();
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize accept fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

void serializeAcceptReply(struct AcceptReply *pat,struct serializeBuff* oa){

	tkJute::AcceptReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->Ok);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeAcceptReply(struct serializeBuff* ia,struct AcceptReply *pat){

	tkJute::AcceptReplyJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ok = pa.ok();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
	}else{
        cout<<"deserialize Accept reply fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

void serializeCommit(struct Commit *pat,struct serializeBuff* oa){

	tkJute::CommitJute pa;
    pa.set_type(pat->Type);
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_seq(pat->Seq);
    for(int i = 0;i < pat->Command.size();i++){
        tkJute::tk_command_jute* cmd = pa.add_command();
        cmd->set_opcode(pat->Command.at(i).opcode);
        cmd->set_key(pat->Command.at(i).key);
        cmd->set_sessioid(pat->Command.at(i).sessionId);
        cmd->set_owner(pat->Command.at(i).owner);
        cmd->set_valsize(pat->Command.at(i).valSize);
        cmd->set_val(pat->Command.at(i).val);
    }
	for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
	}
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeCommit(struct serializeBuff* ia,struct Commit *pat){

	tkJute::CommitJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.command_size();i++){
            struct tk_command cmd;
            cmd.opcode = (OP)pa.command(i).opcode();
            cmd.key = pa.command(i).key();
            cmd.sessionId = pa.command(i).sessioid();
            cmd.owner = pa.command(i).owner();
            cmd.valSize = pa.command(i).valsize();
            cmd.val = (char *)malloc(cmd.valSize);
            memcpy(cmd.val,pa.command(i).val().c_str(),cmd.valSize);
            pat->Command.push_back(cmd);
        }
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize Commit fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();
}

void serializeCommitShort(struct CommitShort *pat,struct serializeBuff* oa){

	tkJute::CommitShortJute pa;
    pa.set_type(pat->Type);
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_count(pat->Count);
	pa.set_seq(pat->Seq);
	for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
	}
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeCommitShort(struct serializeBuff* ia,struct CommitShort *pat){

	tkJute::CommitShortJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Count = pa.count();
        pat->Seq = pa.seq();
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize CommitShort fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();
}

void serializeTryPreAccept(struct TryPreAccept *pat,struct serializeBuff* oa){

	tkJute::TryPreAcceptJute pa;
    pa.set_type(pat->Type);
	pa.set_leaderid(pat->LeaderId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_seq(pat->Seq);
    for(int i = 0;i < pat->Command.size();i++){
        tkJute::tk_command_jute* cmd = pa.add_command();
        cmd->set_opcode(pat->Command.at(i).opcode);
        cmd->set_key(pat->Command.at(i).key);
        cmd->set_sessioid(pat->Command.at(i).sessionId);
        cmd->set_owner(pat->Command.at(i).owner);
        cmd->set_valsize(pat->Command.at(i).valSize);
        cmd->set_val(pat->Command.at(i).val);
    }
    for(int i = 0;i < GROUP_SIZE;i++){
        pa.add_deps(pat->Deps[i]);
    }
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeTryPreAccept(struct serializeBuff* ia,struct TryPreAccept *pat){

	tkJute::TryPreAcceptJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->Seq = pa.seq();
        for(int i = 0;i < pa.command_size();i++){
            struct tk_command cmd;
            cmd.opcode = (OP)pa.command(i).opcode();
            cmd.key = pa.command(i).key();
            cmd.sessionId = pa.command(i).sessioid();
            cmd.owner = pa.command(i).owner();
            cmd.valSize = pa.command(i).valsize();
            cmd.val = (char *)malloc(cmd.valSize);
            memcpy(cmd.val,pa.command(i).val().c_str(),cmd.valSize);
            pat->Command.push_back(cmd);
        }
        for(int i = 0;i < GROUP_SIZE;i++){
            pat->Deps[i] = pa.deps(i);
        }
	}else{
        cout<<"deserialize TryPreAccept fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

void serializeTryPreAcceptReply(struct TryPreAcceptReply *pat,struct serializeBuff* oa){

	tkJute::TryPreAcceptReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->Ok);
	pa.set_acceptorid(pat->AcceptorId);
	pa.set_replica(pat->Replica);
	pa.set_instance(pat->Instance);
	pa.set_ballot(pat->Ballot);
	pa.set_confilctreplica(pat->ConflictReplica);
	pa.set_confilctinstance(pat->ConflictInstance);
	pa.set_confilctstatus(pat->ConflictStatus);
	int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
	bool f = pa.SerializeToArray(oa->buff,len);
	oa->len = len;
	google::protobuf::ShutdownProtobufLibrary();

}

void deserializeTryPreAcceptReply(struct serializeBuff* ia,struct TryPreAcceptReply *pat){

	tkJute::TryPreAcceptReplyJute pa;
	bool f = pa.ParseFromArray(ia->buff,ia->len);
	if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ok = pa.ok();
        pat->AcceptorId = pa.acceptorid();
        pat->Replica = pa.replica();
        pat->Instance = pa.instance();
        pat->Ballot = pa.ballot();
        pat->ConflictReplica = pa.confilctreplica();
        pat->ConflictInstance = pa.confilctinstance();
        pat->ConflictStatus = pa.confilctstatus();
	}else{
        cout<<"deserialize TryPreAcceptReply fail"<<endl;
	}
	google::protobuf::ShutdownProtobufLibrary();

}

//serialize & deserialize Propose
void serializePropose(struct Propose *pat,struct serializeBuff* oa){
    tkJute::ProposeJute pa;
    pa.set_type(pat->Type);
    pa.set_commandid(pat->CommandId);
    pa.set_timestamp(pat->Timestamp);
    tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
    cmd->set_opcode(pat->Command.opcode);
    cmd->set_key(pat->Command.key);
    cmd->set_sessioid(pat->Command.sessionId);
    cmd->set_owner(pat->Command.owner);
    cmd->set_valsize(pat->Command.valSize);
    cmd->set_val(pat->Command.val);
    pa.set_allocated_command(cmd);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializePropose(struct serializeBuff* ia,struct Propose *pat){
    tkJute::ProposeJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->CommandId = pa.commandid();
        pat->Timestamp = pa.timestamp();
        struct tk_command cmd;
        cmd.opcode = (OP)pa.command().opcode();
        cmd.key = pa.command().key();
        cmd.sessionId = pa.command().sessioid();
        cmd.owner = pa.command().owner();
        cmd.valSize = pa.command().valsize();
        cmd.val = (char *)malloc(cmd.valSize);
        memcpy(cmd.val,pa.command().val().c_str(),cmd.valSize);
        pat->Command = cmd;
    }else{
        cout<<"deserialize Propose fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}

//serialize & deserialize ProposeReply
void serializeProposeReply(struct ProposeReply *pat,struct serializeBuff* oa){
    tkJute::ProposeReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->OK);
    pa.set_commandid(pat->CommandId);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeProposeReply(struct serializeBuff* ia,struct ProposeReply *pat){
    tkJute::ProposeReplyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->OK = pa.ok();
        pat->CommandId = pa.commandid();
    }else{
        cout<<"deserialize ProposeReply fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize ProposeReplyTS
void serializeProposeReplyTS(struct ProposeReplyTS *pat,struct serializeBuff* oa){
    tkJute::ProposeReplyTSJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->OK);
    pa.set_commandid(pat->CommandId);
    pa.set_valuesize(pat->ValueSize);
    pa.set_value(pat->Value);
    pa.set_timestamp(pat->Timestamp);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeProposeReplyTS(struct serializeBuff* ia,struct ProposeReplyTS *pat){
    tkJute::ProposeReplyTSJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->OK = pa.ok();
        pat->CommandId = pa.commandid();
        int32_t size = pa.valuesize();
        pat->ValueSize = size;
        pat->Value = (char*)malloc(size);
        memcpy(pat->Value,pa.value().c_str(),size);
        pat->Timestamp = pa.timestamp();
    }else{
        cout<<"deserialize ProposeReplyTS fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize Read
void serializeRead(struct Read *pat,struct serializeBuff* oa){
    tkJute::ReadJute pa;
    pa.set_type(pat->Type);
    pa.set_commandid(pat->CommandId);
    pa.set_key(pat->Key);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeRead(struct serializeBuff* ia,struct Read *pat){
    tkJute::ReadJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->CommandId = pa.commandid();
        pat->Key = pa.key();
    }else{
        cout<<"deserialize Read fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize ProposeAndRead
void serializeProposeAndRead(struct ProposeAndRead *pat,struct serializeBuff* oa){
    tkJute::ProposeAndReadJute pa;
    pa.set_type(pat->Type);
    pa.set_commandid(pat->CommandId);
    pa.set_key(pat->Key);
    tkJute::tk_command_jute* cmd = new tkJute::tk_command_jute;
    cmd->set_opcode(pat->Command.opcode);
    cmd->set_key(pat->Command.key);
    cmd->set_sessioid(pat->Command.sessionId);
    cmd->set_owner(pat->Command.owner);
    cmd->set_valsize(pat->Command.valSize);
    cmd->set_val(pat->Command.val);
    pa.set_allocated_command(cmd);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeProposeAndRead(struct serializeBuff* ia,struct ProposeAndRead *pat){
    tkJute::ProposeAndReadJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->CommandId = pa.commandid();
        pat->Key = pa.key();
        struct tk_command cmd;
        cmd.opcode = (OP)pa.command().opcode();
        cmd.key = pa.command().key();
        cmd.sessionId = pa.command().sessioid();
        cmd.owner = pa.command().owner();
        cmd.valSize = pa.command().valsize();
        cmd.val = (char *)malloc(cmd.valSize);
        memcpy(cmd.val,pa.command().val().c_str(),cmd.valSize);
        pat->Command = cmd;
    }else{
        cout<<"deserialize ProposeAndRead fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize InstanceId
void serializeInstanceId(struct InstanceId *pat,struct serializeBuff* oa){
    tkJute::InstanceIdJute pa;
    pa.set_type(pat->Type);
    pa.set_replica(pat->replica);
    pa.set_instance(pat->instance);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeInstanceId(struct serializeBuff* ia,struct InstanceId *pat){
    tkJute::InstanceIdJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->replica = pa.replica();
        pat->instance = pa.instance();
    }else{
        cout<<"deserialize InstanceId fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize Clock
void serializeClock(struct Clock *pat,struct serializeBuff* oa){
    tkJute::ClockJute pa;
    pa.set_type(pat->Type);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeClock(struct serializeBuff* ia,struct Clock *pat){
    tkJute::ClockJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
    }else{
        cout<<"deserializeClock fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize Beacon_msg
void serializeBeacon_msg(struct Beacon_msg *pat,struct serializeBuff* oa){
    tkJute::Beacon_msgJute pa;
    pa.set_type(pat->Type);
    pa.set_rid(pat->Rid);
    pa.set_timestamp(pat->timestamp);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeBeacon_msg(struct serializeBuff* ia,struct Beacon_msg *pat){
    tkJute::Beacon_msgJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->Rid = pa.rid();
        pat->timestamp = pa.timestamp();
    }else{
        cout<<"deserializeBeacon_msg fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize Beacon_msg_reply
void serializeBeacon_msg_reply(struct Beacon_msg_reply *pat,struct serializeBuff* oa){
    tkJute::Beacon_msg_replyJute pa;
    pa.set_type(pat->Type);
    pa.set_timestamp(pat->timestamp);

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeBeacon_msg_reply(struct serializeBuff* ia,struct Beacon_msg_reply *pat){
    tkJute::Beacon_msg_replyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->timestamp = pa.timestamp();
    }else{
        cout<<"deserialize beacon_msg_reply fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize ClientConnect
void serializeClientConnect(struct ClientConnect *pat,struct serializeBuff* oa){
    tkJute::ClientConnectJute pa;
    pa.set_type(pat->Type);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeClientConnect(struct serializeBuff* ia,struct ClientConnect *pat){
    tkJute::ClientConnectJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
    }else{
        cout<<"deserialize clientconnect fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}

//serialize & deserialize RegisterArgs
void serializeRegisterArgs(struct RegisterArgs *pat,struct serializeBuff* oa){
    tkJute::RegisterArgsJute pa;
    pa.set_type(pat->Type);
    pa.set_addr(pat->Addr);
    pa.set_port(pat->Port);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeRegisterArgs(struct serializeBuff* ia,struct RegisterArgs *pat){
    tkJute::RegisterArgsJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->Addr = pa.addr();
        pat->Port = pa.port();
    }else{
        cout<<"deserialize registerargs fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize RegisterReply
void serializeRegisterReply(struct RegisterReply *pat,struct serializeBuff* oa){
    tkJute::RegisterReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_replicaid(pat->ReplicaId);
    pa.set_ready(pat->Ready);
    for(int i = 0;i < pat->AddrList.size();i++){
        pa.add_addrlist(pat->AddrList.at(i).c_str());
    }
    for(int i = 0;i < pat->PortList.size();i++){
        pa.add_portlist(pat->PortList.at(i));
    }
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeRegisterReply(struct serializeBuff* ia,struct RegisterReply *pat){
    tkJute::RegisterReplyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->ReplicaId = pa.replicaid();
        pat->Ready = pa.ready();
        for(int i = 0;i < pa.addrlist().size();i++){
            pat->AddrList.push_back(pa.addrlist()[i]);
        }
        for(int i = 0;i < pa.portlist().size();i++){
            pat->PortList.push_back(pa.portlist()[i]);
        }
    }else{
        cout<<"deserialize registerreply fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize GetLeaderArgs
void serializeGetLeaderArgs(struct GetLeaderArgs *pat,struct serializeBuff* oa){
    tkJute::GetLeaderArgsJute pa;
    pa.set_type(pat->Type);

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeGetLeaderArgs(struct serializeBuff* ia,struct GetLeaderArgs *pat){
    tkJute::GetLeaderArgsJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
    }else{
        cout<<"deserialize getleaderargs fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize GetLeaderReply
void serializeGetLeaderReply(struct GetLeaderReply *pat,struct serializeBuff* oa){
    tkJute::GetLeaderReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_leaderid(pat->LeaderId);

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeGetLeaderReply(struct serializeBuff* ia,struct GetLeaderReply *pat){
    tkJute::GetLeaderReplyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->LeaderId = pa.leaderid();
    }else{
        cout<<"deserialize getleaderreply fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize GetReplicaListArgs
void serializeGetReplicaListArgs(struct GetReplicaListArgs *pat,struct serializeBuff* oa){
    tkJute::GetReplicaListArgsJute pa;
    pa.set_type(pat->Type);

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeGetReplicaListArgs(struct serializeBuff* ia,struct GetReplicaListArgs *pat){
    tkJute::GetReplicaListArgsJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
    }else{
        cout<<"deserialize getreplicalistargs fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize GetReplicaListReply
void serializeGetReplicaListReply(struct GetReplicaListReply *pat,struct serializeBuff* oa){
    tkJute::GetReplicaListReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ready(pat->Ready);
    for(int i = 0;i < pat->ReplicaAddrList.size();i++){
        pa.add_replicaaddrlist(pat->ReplicaAddrList.at(i).c_str());
    }
    for(int i = 0;i < pat->ReplicaPortList.size();i++){
        pa.add_replicaportlist(pat->ReplicaPortList.at(i));
    }

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeGetReplicaListReply(struct serializeBuff* ia,struct GetReplicaListReply *pat){
    tkJute::GetReplicaListReplyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ready = pa.ready();
        for(int i = 0;i < pa.replicaaddrlist().size();i++){
            pat->ReplicaAddrList.push_back(pa.replicaaddrlist()[i]);
        }
        for(int i = 0;i < pa.replicaportlist().size();i++){
            pat->ReplicaPortList.push_back(pa.replicaportlist()[i]);
        }
    }else{
        cout<<"deserialize getreplicalistreply fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize BeTheLeaderReply
void serializeBeTheLeaderReply(struct BeTheLeaderReply *pat,struct serializeBuff* oa){
    tkJute::BeTheLeaderReplyJute pa;
    pa.set_type(pat->Type);
    pa.set_ok(pat->Ok);
    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeBeTheLeaderReply(struct serializeBuff* ia,struct BeTheLeaderReply *pat){
    tkJute::BeTheLeaderReplyJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
        pat->Ok = pa.ok();
    }else{
        cout<<"deserialize be the leader fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}
//serialize & deserialize GENERAL
void serializeGENERAL(struct GENERAL *pat,struct serializeBuff* oa){
    tkJute::GENERALJute pa;
    pa.set_type(pat->Type);

    int64_t len = pa.ByteSize();
    oa->buff = (char*)malloc(len);
    bool f = pa.SerializeToArray(oa->buff,len);
    oa->len = len;
    google::protobuf::ShutdownProtobufLibrary();
}
void deserializeGENERAL(struct serializeBuff* ia,struct GENERAL *pat){
    tkJute::TryPreAcceptJute pa;
    bool f = pa.ParseFromArray(ia->buff,ia->len);
    if(f){
        pat->Type = (TYPE)pa.type();
    }else{
        cout<<"deserialize general fail"<<endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
}

