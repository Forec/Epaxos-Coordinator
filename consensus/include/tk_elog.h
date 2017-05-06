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
    RecoverInstance() {};
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
    int tpaOks;
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
    LeaderBookkeeping() {
    };
    LeaderBookkeeping(std::vector<Propose> & _proposals, int32_t _maxRecv, int _prepareOKs,
                      bool _allEqual, int _preAcceptOKs, int _acceptOKs, int _nacks,
                      std::array<int32_t, GROUP_SZIE> & _originalDeps,
                      std::vector<int32_t> _committedDeps,
                      RecoverInstance * _recov,
                      bool _preparing, bool _tryingToPreAccept,
                      std::vector<bool> & _possibleQuorum, int _tapOKs):
        clientProposals(_proposals), maxRecvBallot(_maxRecv), prepareOKs(_prepareOKs),
        allEqual(_allEqual), preAcceptOKs(_preAcceptOKs), acceptOKs(_acceptOKs), nacks(_nacks),
        originalDeps(_originalDeps), committedDeps(_committedDeps), recoveryInst(_recov),
        preparing(_preparing), tryingToPreAccept(_tryingToPreAccept), possibleQuorum(_possibleQuorum),
        tpaOks(_tapOKs){
    };
};

typedef struct LeaderBookkeeping lb_t;

struct tk_instance {
    int32_t  ballot;
    STATUS status;
    int32_t  seq;
    std::vector<tk_command> cmds;
    std::array<int32_t, GROUP_SZIE> deps;
    lb_t * lb;

    // dfn and low: for tarjan algorithm in execution loop
    unsigned short dfn;
    unsigned short low;
    tk_instance(){
        dfn = low = 0;
    };
    tk_instance(std::vector<tk_command> & _cmds, int32_t _ballot, STATUS _status,
                int32_t _seq, std::array<int32_t, GROUP_SZIE> & _deps, lb_t * _lb) :
        cmds(_cmds), ballot(_ballot), status(_status), seq(_seq), deps(_deps), lb(_lb){
        dfn = low = 0;
    };
};

#endif //TKDATABASE_TK_ELOG_H
