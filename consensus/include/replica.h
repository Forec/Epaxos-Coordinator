//
// Created by forec on 17-4-22.
//

#ifndef TKDATABASE_REPLICA_H
#define TKDATABASE_REPLICA_H

#include "debug.h"
#include "tk_message.h"
#include "tk_elog.h"
#include "../../config/config.h"
#include "../../include/Tkdatabase.h"
#include "../../utils/include/msg_queue.h"
#include <vector>
#include <string>
#include <unordered_map>

struct Replica {
    int32_t Id;
    std::vector<std::string> PeerAddrList;
    // TODO: std::vector<RDMA Handle> Peers
    bool Durable;
    bool Shutdown;
    bool Exec;
    bool Dreply;
    bool Beacon;
    bool Restore;
    unsigned long group_size;
    uint64_t checkpoint_cycle;

    std::vector<bool> Alive;
    std::vector<int32_t> PreferredPeerOrder;
    std::vector<double> Ewma;
    std::vector<int32_t> crtInstance;
    std::array<int32_t, GROUP_SZIE> committedUpTo;
    std::vector<int32_t> executeUpTo;
    std::vector<std::unordered_map<std::string, int32_t>> conflicts;
    std::unordered_map<std::string, int32_t> maxSeqPerKey;

    std::string path;
    Tkdatabase_t* statemachine;
    FILE *log_fp;
    tk_instance *** InstanceMatrix;

    MsgQueue_t * mq;
    MsgQueue_t * pro_mq;

    int32_t maxSeq;
    int32_t latestCPReplica;
    int32_t latestCPInstance;


    // METHODS
    bool verify();
    Replica() {};
    bool init();
    bool run();
};

void connectToPeers(Replica * param);
void updatePreferredPeerOrder(Replica * r);

// threads TODO
void * waitForClientConnections(void * arg);
void * stopAdapting(void * arg);
void * slowClock(void * arg);
void * fastClock(void * arg);


// handlers for different msg TODO
void handlePropose(Replica * r, Propose * msgp);
void handlePrepare(Replica * r, Prepare * msgp);
void handlePreAccept(Replica * r, PreAccept * msgp);
void handleAccept(Replica * r, Accept * msgp);
void handleCommit(Replica * r, Commit * msgp);
void handleCommitShort(Replica * r, CommitShort * msgp);
void handlePrepareReply(Replica * r, PrepareReply * msgp);
void handlePreAcceptReply(Replica * r, PreAcceptReply * msgp);
void handlePreAcceptOK(Replica * r, PreAcceptOk * msgp);
void handleAcceptReply(Replica * r, AcceptReply * msgp);
void handleTryPreAccept(Replica * r, TryPreAccept * msgp);
void handleTryPreAcceptReply(Replica * r, TryPreAcceptReply * msgp);
void replyBeacon(Replica * r, Beacon_msg * beacon);
void sendBeacon(Replica * r, int id);
void startRecoveryForInstance(Replica * r, InstanceId * iid);

// Phase 1
void startPhase1(Replica * r, uint64_t instance,
                 uint32_t ballot, Propose * proposals,
                 tk_command * cmds, long batchSize);

// Helper functions
uint8_t updateAttributes(Replica * r, long cmds_len, tk_command * cmds,
                      unsigned int * seq, unsigned int * deps, int replica, uint64_t instance);
void updateConflicts(Replica * r, long len, tk_command * cmds, uint8_t replica,
                     uint64_t instance, unsigned int seq);


// inter replica communications
void bcastPreAccept(Replica * r, uint8_t replica, uint64_t instance,
                    uint32_t ballot, tk_command * cmds, unsigned int seq, unsigned int * deps);


// recovery actions
void replyPrepare(Replica * r, int32_t LeaderId, PrepareReply * preply);

extern int conflict, weird, slow, happy;
extern int cpcounter;

#endif //TKDATABASE_REPLICA_H
