//
// Created by jingle on 17-3-24.
//

#ifndef TKDATABASE_TK_SERVER_H
#define TKDATABASE_TK_SERVER_H

#include "tk_elog.h"


#include "debug.h"
#include "Tkdatanode.h"

#define BATCH_INTERVAL 50
#define TIMEOUT_INTERVAL 50
#define EXECUTE_INTERVAL 50
#define GROUP_SZIE  3
#define CHECKPOINT_CYCLE 1024

#define PORT 1111

#define PERSISTENT_MASK 0x01
#define BATCHING_MASK 0x02
#define REPLY_MASK 0x04  // reply to client after command has been executed?
#define RESTORE_MASK 0x08
#define SHUTDOWN_MASK 0x10


//struct replica_server{
//
//    uint8_t replicaId;
//    uint8_t group_size;
//    uint64_t* MaxInstanceNum; // the highest instance number seen for each replica;
//    uint64_t ProposeNum;  // how to use this;
//    Tkdatabase_t* statemachine; // State Machine;
//    tk_instance_t ** InstanceMatrix; //　Instance Matrix,
//    uint32_t  epoch; //  Epoch;
//    uint64_t* excutedupto;
//    char ** addrs;  // address of other servers;
//    bool enablepersistent;
//    FILE *log_fp;
//    bool execute_trigger;
//    uint64_t CheckpointCycle;
//    bool enablebatching;
//
//};

typedef struct replica_server replica_server_t;


struct pack_replica {

    uint8_t Id;
    uint8_t group_size;
    uint64_t* MaxInstanceNum;
    uint64_t* excutedupto;
    uint64_t proposeNum;
};

typedef struct pack_replica pack_replica_t;



struct propose_request {
    tk_command_t cmd;
    uint64_t  id;
};

typedef struct propose_request propose_request_t;

struct replica_server_param{
    uint8_t replicaId;
    uint8_t group_size;
    Tkdatabase_t* statemachine;
    uint64_t* MaxInstanceNum;
    uint64_t* executeupto;
    char ** addrs;
    uint8_t flag;
    FILE *log_fp;
    uint64_t ProposeNum;
    tk_instance_t** InstanceMatrix;
    char *string_path;
    uint64_t checkpoint_cycle;
};

typedef struct replica_server_param replica_server_param_t;











#endif //TKDATABASE_TK_SERVER_H
