//
// Created by forec on 17-5-4.
//

#ifndef TKDATABASE_CONFIG_H
#define TKDATABASE_CONFIG_H


#define BATCH_INTERVAL 50
#define TIMEOUT_INTERVAL 50
#define EXECUTE_INTERVAL 50
#define GROUP_SZIE 3
#define CHECKPOINT_CYCLE 1024

#define SERVER_PORT 9001
#define MASTER_PORT 7087

#define MAX_BATCH 1000
#define ADAPT_TIME_SEC 10
#define DO_CHECKPOINTING false
#define CHECKPOINT_PERIOD 10000

enum OP {
    PUT = 0,
    GET
};

enum STATUS {
    NONE = 0,               // 0
    PREACCEPTED,            // 1
    PREACCEPTED_EQ,         // 2
    ACCEPTED,               // 3
    COMMITTED,              // 4
    EXECUTED                // 5
};

enum TYPE {
    DEFAULT = 0,
    READ,                   // 1
    PROPOSE,                // 2
    PROPOSE_AND_READ,       // 3
    PROPOSE_REPLY,          // 4
    PROPOSE_REPLY_TS,       // 5
    PREPARE,                // 6
    PREACCEPT,              // 7
    ACCEPT,                 // 8
    COMMIT,                 // 9
    COMMIT_SHORT,           // 10
    PREPARE_REPLY,          // 11
    PREACCEPT_REPLY,        // 12
    PREACCEPT_OK,           // 13
    ACCEPT_REPLY,           // 14
    TRY_PREACCEPT,          // 15
    TRY_PREACCEPT_REPLY,    // 16
    BEACON,                 // 17
    BEACON_REPLY,           // 18
    FAST_CLOCK,             // 19
    SLOW_CLOCK,             // 20
    CLIENT_CONNECT,         // 21
    RECOVER_INSTANCE,       // 22
    INSTANCE_ID,            // 23
    REGISTER_ARGS,          // 24
    REGISTER_REPLY,         // 25
    GET_LEADER_ARGS,        // 26
    GET_LEADER_REPLY,       // 27
    GET_REPLICA_LIST_ARGS,  // 28
    GET_REPLICA_LIST_REPLY, // 29
    PING,                   // 30
    PING_REPLY,             // 31
    BE_LEADER,              // 32
    BE_LEADER_REPLY         // 33
};

#define RDMA_CONNECTION int

#endif //TKDATABASE_CONFIG_H
