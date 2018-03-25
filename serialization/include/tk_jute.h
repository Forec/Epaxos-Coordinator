#ifndef TK_JUTE_INCLUDED
#define TK_JUTE_INCLUDED
#include "../../consensus/include/tk_message.h"
#include "../include/protobuff.pb.h"

struct serializeBuff {
    int32_t len;
    void* buff;
};

//serialize & deserialize tk_command
void serializeTkCommand(struct tk_command * command,serializeBuff * oa);
void deserializeTkCommand(struct serializeBuff * ia,struct tk_command * command);
//serialize & deserialize prepare
void serializePrepare(struct Prepare *pat,struct serializeBuff* oa);
void deserializePrepare(struct serializeBuff* ia,struct Prepare *pat);
//serialize & deserialize PrepareReply
void serializePrepareReply(struct PrepareReply *pat,struct serializeBuff* oa);
void deserializePrepareReply(struct serializeBuff* ia,struct  PrepareReply *pat);
//serialize & deserialize PreAccept
void serializePreAccept(struct PreAccept *pat,struct serializeBuff* oa);
void deserializePreAccept(struct serializeBuff* ia,struct PreAccept *pat);
//serialize & deserialize PreAcceptReply
void serializePreAcceptReply(struct PreAcceptReply *pat,struct serializeBuff* oa);
void deserializePreAcceptReply(struct serializeBuff* ia,struct PreAcceptReply *pat);
//serialize & deserialize PreAcceptOk
void serializePreAcceptOk(struct PreAcceptOk *pat,struct serializeBuff* oa);
void deserializePreAcceptOk(struct serializeBuff* ia,struct PreAcceptOk *pat);
//serialize & deserialize Accept
void serializeAccept(struct Accept *pat,struct serializeBuff* oa);
void deserializeAccept(struct serializeBuff* ia,struct Accept *pat);
//serialize & deserialize AcceptReply
void serializeAcceptReply(struct AcceptReply *pat,struct serializeBuff* oa);
void deserializeAcceptReply(struct serializeBuff* ia,struct AcceptReply *pat);
//serialize & deserilize Commit
void serializeCommit(struct Commit *pat,struct serializeBuff* oa);
void deserializeCommit(struct serializeBuff* ia,struct Commit *pat);
//serialize & deserializ CommitShort
void serializeCommitShort(struct CommitShort *pat,struct serializeBuff* oa);
void deserializeCommitShort(struct serializeBuff* ia,struct CommitShort *pat);
//serialize & deserialize TryPreAccept
void serializeTryPreAccept(struct TryPreAccept *pat,struct serializeBuff* oa);
void deserializeTryPreAccept(struct serializeBuff* ia,struct TryPreAccept *pat);
//serialize & deserialize TryPreAcceptReply
void serializeTryPreAcceptReply(struct TryPreAcceptReply *pat,struct serializeBuff* oa);
void deserializeTryPreAcceptReply(struct serializeBuff* ia,struct TryPreAcceptReply *pat);
//serialize & deserialize Propose
void serializePropose(struct Propose *pat,struct serializeBuff* oa);
void deserializePropose(struct serializeBuff* ia,struct Propose *pat);
//serialize & deserialize ProposeReply
void serializeProposeReply(struct ProposeReply *pat,struct serializeBuff* oa);
void deserializeProposeReply(struct serializeBuff* ia,struct ProposeReply *pat);
//serialize & deserialize ProposeReplyTS
void serializeProposeReplyTS(struct ProposeReplyTS *pat,struct serializeBuff* oa);
void deserializeProposeReplyTS(struct serializeBuff* ia,struct ProposeReplyTS *pat);
//serialize & deserialize Read
void serializeRead(struct Read *pat,struct serializeBuff* oa);
void deserializeRead(struct serializeBuff* ia,struct Read *pat);
//serialize & deserialize ProposeAndRead
void serializeProposeAndRead(struct ProposeAndRead *pat,struct serializeBuff* oa);
void deserializeProposeAndRead(struct serializeBuff* ia,struct ProposeAndRead *pat);
//serialize & deserialize InstanceId
void serializeInstanceId(struct InstanceId *pat,struct serializeBuff* oa);
void deserializeInstanceId(struct serializeBuff* ia,struct InstanceId *pat);
//serialize & deserialize Clock
void serializeClock(struct Clock *pat,struct serializeBuff* oa);
void deserializeClock(struct serializeBuff* ia,struct Clock *pat);
//serialize & deserialize Beacon_msg
void serializeBeacon_msg(struct Beacon_msg *pat,struct serializeBuff* oa);
void deserializeBeacon_msg(struct serializeBuff* ia,struct Beacon_msg *pat);
//serialize & deserialize Beacon_msg_reply
void serializeBeacon_msg_reply(struct Beacon_msg_reply *pat,struct serializeBuff* oa);
void deserializeBeacon_msg_reply(struct serializeBuff* ia,struct Beacon_msg_reply *pat);
//serialize & deserialize ClientConnect
void serializeClientConnect(struct ClientConnect *pat,struct serializeBuff* oa);
void deserializeClientConnect(struct serializeBuff* ia,struct ClientConnect *pat);
//serialize & deserialize RegisterArgs
void serializeRegisterArgs(struct RegisterArgs *pat,struct serializeBuff* oa);
void deserializeRegisterArgs(struct serializeBuff* ia,struct RegisterArgs *pat);
//serialize & deserialize RegisterReply
void serializeRegisterReply(struct RegisterReply *pat,struct serializeBuff* oa);
void deserializeRegisterReply(struct serializeBuff* ia,struct RegisterReply *pat);
//serialize & deserialize GetLeaderArgs
void serializeGetLeaderArgs(struct GetLeaderArgs *pat,struct serializeBuff* oa);
void deserializeGetLeaderArgs(struct serializeBuff* ia,struct GetLeaderArgs *pat);
//serialize & deserialize GetLeaderReply
void serializeGetLeaderReply(struct GetLeaderReply *pat,struct serializeBuff* oa);
void deserializeGetLeaderReply(struct serializeBuff* ia,struct GetLeaderReply *pat);
//serialize & deserialize GetReplicaListArgs
void serializeGetReplicaListArgs(struct GetReplicaListArgs *pat,struct serializeBuff* oa);
void deserializeGetReplicaListArgs(struct serializeBuff* ia,struct GetReplicaListArgs *pat);
//serialize & deserialize GetReplicaListReply
void serializeGetReplicaListReply(struct GetReplicaListReply *pat,struct serializeBuff* oa);
void deserializeGetReplicaListReply(struct serializeBuff* ia,struct GetReplicaListReply *pat);
//serialize & deserialize BeTheLeaderReply
void serializeBeTheLeaderReply(struct BeTheLeaderReply *pat,struct serializeBuff* oa);
void deserializeBeTheLeaderReply(struct serializeBuff* ia,struct BeTheLeaderReply *pat);
//serialize & deserialize GENERAL
void serializeGENERAL(struct GENERAL *pat,struct serializeBuff* oa);
void deserializeGENERAL(struct serializeBuff* ia,struct GENERAL *pat);

#endif // TK_JUTE_INCLUDED


