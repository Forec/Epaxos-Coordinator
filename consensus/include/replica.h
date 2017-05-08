//
// Created by forec on 17-4-22.
//

#ifndef TKDATABASE_REPLICA_H
#define TKDATABASE_REPLICA_H

#include "debug.h"
#include "tk_message.h"
#include "tk_elog.h"
#include "../../config/config.h"
#include "../../database/include/Tkdatabase.h"
#include "../../utils/include/msg_queue.h"
#include "../../utils/include/utils.h"
#include "../../utils/include/communication.h"
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <unordered_map>

struct Replica {
    int32_t Id;
    bool Durable;
    bool Shutdown;
    bool Exec;
    bool Dreply;
    bool Thrifty;
    bool Beacon;
    bool Restore;
    int group_size;
    int ListeningPort;
    uint64_t checkpoint_cycle;

    std::vector<bool> Alive;
    std::vector<int32_t> PreferredPeerOrder;
    std::vector<double> Ewma;
    std::vector<int32_t> crtInstance;
    std::array<int32_t, GROUP_SZIE> committedUpTo;
    std::vector<int32_t> executeUpTo;
    std::vector<int32_t> PeerPortList;
    std::vector<std::string> PeerAddrList;
    std::vector<std::unordered_map<std::string, int32_t>> conflicts;
    std::unordered_map<std::string, int32_t> maxSeqPerKey;

    std::string path;
    Tkdatabase* statemachine;
    FILE *log_fp;
    tk_instance *** InstanceMatrix;

    MsgQueue * mq;
    MsgQueue * pro_mq;

    int32_t maxSeq;
    int32_t latestCPReplica;
    int32_t latestCPInstance;

    // TODO
     RDMA_CONNECTION Listener;
     std::vector<RDMA_CONNECTION> Peers;

    Replica();
    Replica(int32_t _rId, std::vector<std::string> & _addrList, std::vector<int> & _portList,
            bool _thrifty, bool _exec, bool _dreply, bool _beacon, bool _durable);
    bool verify();
    bool init();
    bool run();

    // Phase 1
    void startPhase1(int32_t replica, int32_t instance, int32_t ballot,
                     std::vector<Propose> & proposals,
                     std::vector<tk_command> & cmds, long batchSize);
    void handlePropose(Propose * propose);
    void handlePreAccept(PreAccept * preAccept);
    void handlePreAcceptReply(PreAcceptReply * pareply);
    void handlePreAcceptOK(PreAcceptOk * preacok);

    // Phase 2
    void handleAccept(Accept * accept);
    void handleAcceptReply(AcceptReply * acreply);

    // Phase Commit
    void handleCommit(Commit * commit);
    void handleCommitShort(CommitShort * commit);

    // Phase recovery
    void handlePrepare(Prepare * prepare);
    void handlePrepareReply(PrepareReply * preply);
    void handleTryPreAccept(TryPreAccept * tpap);
    void handleTryPreAcceptReply(TryPreAcceptReply * tpar);
    void startRecoveryForInstance(int32_t replica, int32_t instance);
    bool findPreAcceptConflicts(std::vector<tk_command> & cmds, int32_t replica, int32_t instance,
                                int32_t seq, std::array<int32_t, GROUP_SZIE> & deps, int *q, int *i);

    // Helper functions
    void updatePreferredPeerOrder(std::vector<int32_t> & quorum);
    bool updateAttributes(std::vector<tk_command> &cmds, int32_t &seq,
                          std::array<int32_t, GROUP_SZIE> & deps,
                          int32_t replica, int32_t instance);
    bool mergeAttributes(int32_t & seq1, std::array<int32_t, GROUP_SZIE> & deps1,
                         int32_t seq2, std::array<int32_t, GROUP_SZIE> & deps2);
    bool ConflictBatch(std::vector<tk_command> & batch1, std::vector<tk_command> & batch2);
    bool Conflict(tk_command & delta, tk_command & gamma);

    void updateConflicts(std::vector<tk_command> &cmds, int32_t replica,
                         int32_t instance, int32_t seq);
    void updateCommitted(int32_t replica);
    void clearHashTables();
    int32_t makeUniqueBallot(int32_t ballot);
    int32_t makeBallotLargerThan(int32_t ballot);

    // inter replica communications
    void connectToPeers(std::vector<std::thread *> & threads);
    void bcastPreAccept(int32_t replica, int32_t instance, uint32_t ballot,
                        std::vector<tk_command> &cmds, int32_t seq,
                        std::array<int32_t, GROUP_SZIE> & deps);
    void bcastAccept(int32_t replica, int32_t instance, int32_t ballot, int32_t count,
                     int32_t seq, std::array<int32_t, GROUP_SZIE> & deps);
    void bcastCommit(int32_t replica, int32_t instance, std::vector<tk_command> & cmds,
                     int32_t seq, std::array<int32_t, GROUP_SZIE> & deps);
    void bcastPrepare(int32_t replica, int32_t instance, int32_t ballot);
    void bcastTryPreAccept(int32_t replica, int32_t instance, int32_t ballot,
                           std::vector<tk_command> &cmds, int32_t seq,
                           std::array<int32_t, GROUP_SZIE> & deps);

    // reply methods
    void replyPrepare(int32_t LeaderId, PrepareReply * preply);
    void replyPreAccept(int32_t LeaderId, PreAcceptReply * preacply);
    void replyAccept(int32_t LeaderId, AcceptReply * acr);
    void replyTryPreAccept(int32_t LeaderId, TryPreAcceptReply * ntpap);
    void replyBeacon(Beacon_msg * beacon);
    void sendBeacon(int32_t peerId);
};

// threads
void waitForClientConnections(Replica * r);
void clientListener(Replica * r, RDMA_CONNECTION conn);
void waitForPeerConnections(Replica * r, bool * done);
void replicaListener(Replica *r, int32_t rid, RDMA_CONNECTION conn);
void stopAdapting(Replica * r);
void slowClock(Replica * r);
void fastClock(Replica * r);

// helper functions
void updateDeferred(int32_t dr, int32_t di, int32_t r, int32_t i);
bool deferredByInstance(int32_t q, int32_t i, int32_t * dq, int32_t *di);
uint64_t CPUTicks();
void LEPutUint32(char * buf, int32_t v);
void msgDispatcher(Replica * r, int32_t rid, RDMA_CONNECTION conn, TYPE msgType);

extern int conflict, weird, slow, happy;
extern int cpcounter;
extern std::vector<tk_command> emptyVec_cmd;
extern std::vector<Propose> emptyVec_pro;
extern std::vector<int32_t> emptyVec_int;
extern std::vector<bool> emptyVec_bool;
extern std::unordered_map<uint64_t, uint64_t > deferMap;

extern PreAccept pa;
extern TryPreAccept tpa;
extern Prepare pr;
extern Accept ea;
extern Commit cm;
extern CommitShort cms;

#endif //TKDATABASE_REPLICA_H
