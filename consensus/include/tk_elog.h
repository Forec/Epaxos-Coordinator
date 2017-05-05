//
// Created by jingle on 17-3-22.
//

#ifndef TKDATABASE_TK_ELOG_H
#define TKDATABASE_TK_ELOG_H

#include <stdint.h>
#include <array>
#include <vector>
#include "../../config/config.h"
#include "tk_command.h"
#include "tk_message.h"

struct RecoverInstance {
    bool leaderResponded;
    int8_t status;
    int32_t seq;
    int preAcceptCount;
    std::array<int32_t, GROUP_SZIE> deps;
    std::vector<tk_command> cmds;
    RecoverInstance(bool _leaderResponded, int8_t _status,
                    int32_t _seq, int _preAcceptCount,
                    std::array<int32_t, GROUP_SZIE> & _deps,
                    std::vector<tk_command> & _cmds) :
        leaderResponded(_leaderResponded), status(_status),
        seq(_seq), preAcceptCount(_preAcceptCount), deps(_deps), cmds(_cmds) {
    };
};

struct LeaderBookkeeping {
    int32_t maxRecvBallot;
    int prepareOKs;
    bool allEqual;
    int preAcceptOKs;
    int acceptOKs;
    int nacks;
    std::array<int32_t, GROUP_SZIE> originalDeps;
    std::vector<int32_t> committedDeps;
    RecoverInstance * recoveryInst;
    bool preparing;
    bool tryingToPreAccept;
    // TODO::Should not use vector<bool> here
    std::vector<bool> possibleQuorum;
    std::vector<Propose> clientProposals;
    int tapOKs;
};

typedef struct LeaderBookkeeping lb_t;

struct tk_instance {
    std::vector<tk_command> cmds;
    int32_t  ballot;
    int8_t  status;
    int32_t  seq;
    std::array<int32_t, GROUP_SZIE> deps;
    lb_t * lb;

    // dfn and low: for tarjan algorithm in execution loop
    unsigned short dfn;
    unsigned short low;
};


#endif //TKDATABASE_TK_ELOG_H
