//
// Created by forec on 17-4-22.
//

#ifndef TKDATABASE_REPLICA_H
#define TKDATABASE_REPLICA_H

#include "debug.h"
#include "tk_elog.h"
#include "../../config/config.h"
#include "../../include/Tkdatabase.h"
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

    int32_t maxSeq;
    int32_t latestCPReplica;
    int32_t latestCPInstance;


    // METHODS
    bool verify();
    Replica() {};
    bool init();
    bool run();
};


#endif //TKDATABASE_REPLICA_H
