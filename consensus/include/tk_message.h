//
// Created by jingle on 17-3-31.
//

#ifndef TKDATABASE_TK_MESSAGE_H
#define TKDATABASE_TK_MESSAGE_H

#include "tk_elog.h"

#define NONE 0
#define PREACCEPTED 1
#define PREACCEPTED_EQ 2
#define ACCEPTED   3
#define COMMITTED  4
#define EXECUTED   5


struct Prepare {

    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;

};

typedef struct Prepare Prepare_t;


struct PrepareReply {

    int32_t AcceptorId;
    int32_t Replica;
    int32_t Instance;
    uint8_t Ok;
    int32_t Ballot;
    int8_t  Status;
    tk_command_t cmd;
    int32_t Seq;
    int32_t Deps[5];

};

typedef struct PrepareReply  PrepareReply_t;


struct PreAccept {
    int32_t LeaderId;
    int32_t Replica;
    int32_t  Instance;
    int32_t Ballot;
    tk_command_t cmd;
    int32_t  Seq;
    int32_t  Deps[5];
};


typedef struct PreAccept PreAccept_t;


struct PreAcceptReply {

    int32_t Replica;
    int32_t Instance;
    uint8_t  Ok;
    int32_t  Ballot;
    int32_t  Seq;
    int32_t  Deps[5];
    int32_t  CommittedDeps[5];
};

typedef struct PreAcceptReply PreAcceptReply_t;


struct PreAcceptOk {

    int32_t Instance;

};

typedef struct PreAcceptOk PreAcceptOk_t;


struct Accept {

    int32_t  LeaderId;
    int32_t  Replica;
    int32_t  Instance;
    int32_t  Ballot;
    int32_t  Count;
    int32_t  Seq;
    int32_t  Deps[5];
};

typedef struct Accept Accept_t;


struct AcceptReply {

    int32_t Replica;
    int32_t Instance;
    uint8_t Ok;
    int32_t Ballot;
};

typedef struct AcceptReply AcceptReply_t;


struct Commit {
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    tk_command_t cmd;
    int32_t Seq;
    int32_t Deps[5];
};

typedef struct Commit Commit_t;

struct CommitShort {
    int32_t  LeaderId;
    int32_t  Replica;
    int32_t  Instance;
    int32_t  Count;
    int32_t  Seq;
    int32_t  Deps[5];
};

typedef struct CommitShort CommitShort_t;


struct TryPreAccept {
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    tk_command_t cmd;
    int32_t Seq;
    int32_t Deps[5];
};

typedef struct TryPreAcceptReply {
    int32_t AcceptorId;
    int32_t Replica;
    int32_t Instance;
    uint8_t Ok;
    int32_t Ballot;
    int32_t ConfilctReplica;
    int32_t ConfilctInstance;
    int32_t ConfilctStatus;
};

typedef struct TryPreAcceptReply TryPreAcceptReply_t;








#endif //TKDATABASE_TK_MESSAGE_H
