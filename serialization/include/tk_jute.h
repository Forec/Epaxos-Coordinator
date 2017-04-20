#ifndef TK_JUTE_INCLUDED
#define TK_JUTE_INCLUDED
#include "../../consensus/include/tk_message.h"
struct serializeBuff {
    int32_t len;
    void* buff;
};

//serialize & deserialize prepare
void serializePrepre(PreAccept_t *pat,struct serializeBuff* oa);
void deserializePrepare(struct serializeBuff* ia,struct Prepare *pat);
//serialize & deserialize PrepareReply
void serializePrepareReply(PrepareReply_t *pat,struct serializeBuff* oa);
void deserializePrepareReply(struct serializeBuff* ia, PrepareReply_t *pat);
//serialize & deserialize PreAccept
void serializePreAccept(PreAccept_t *pat,struct serializeBuff* oa);
void deserializePreAccept(struct serializeBuff* ia,struct PreAccept *pat);
//serialize & deserialize PreAcceptReply
void serializePreAcceptReply(PreAcceptReply_t *pat,struct serializeBuff* oa);
void deserializePreAcceptReply(struct serializeBuff* ia,struct PreAcceptReply *pat);
//serialize & deserialize PreAcceptOk
void serializePreAcceptOk(PreAcceptOk_t *pat,struct serializeBuff* oa);
void deserializePreAcceptOk(struct serializeBuff* ia,struct PreAcceptOk *pat);
//serialize & deserialize Accept
void serializeAccept(Accept_t *pat,struct serializeBuff* oa);
void deserializeAccept(struct serializeBuff* ia,struct Accept *pat);
//serialize & deserialize AcceptReply
void serializeAcceptReply(AcceptReply_t *pat,struct serializeBuff* oa);
void deserializeAcceptReply(struct serializeBuff* ia,struct AcceptReply *pat);
//serialize & deserilize Commit
void serializeCommit(Commit_t *pat,struct serializeBuff* oa);
void deserializeCommit(struct serializeBuff* ia,struct Commit *pat);
//serialize & deserializ CommitShort
void serializeCommitShort(CommitShort_t *pat,struct serializeBuff* oa);
void deserializeCommitShort(struct serializeBuff* ia,struct CommitShort *pat);
//serialize & deserialize TryPreAccept
void serializeTryPreAccept(struct TryPreAccept *pat,struct serializeBuff* oa);
void deserializeTryPreAccept(struct serializeBuff* ia,struct TryPreAccept *pat);
//serialize & deserialize TryPreAcceptRepy=le
void serializeTryPreAcceptReply(TryPreAcceptReply_t *pat,struct serializeBuff* oa);
void deserializeTryPreAcceptReply(struct serializeBuff* ia,struct TryPreAcceptReply *pat);


#endif // TK_JUTE_INCLUDED


