//
// Created by jingle on 17-3-31.
//

#ifndef TKDATABASE_TK_MESSAGE_H
#define TKDATABASE_TK_MESSAGE_H

#include "../../config/config.h"
#include <vector>
#include <array>
#include "tk_command.h"

struct Prepare {
    uint32_t Type;
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    Prepare(int32_t _leaderId, int32_t _replica, int32_t _instance, int32_t _ballot) :
        LeaderId(_leaderId), Replica(_replica), Instance(_instance), Ballot(_ballot){
        Type = PREPARE;
    }
};

struct PrepareReply {
    bool Ok;
    TYPE Type;
    STATUS  Status;
    int32_t AcceptorId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    int32_t Seq;
    std::vector<tk_command> Command;
    std::array<int32_t, GROUP_SZIE> Deps;
    PrepareReply(int32_t _acceptorId, int32_t _replica, int32_t _instance,
                 bool _ok, int32_t _ballot, STATUS _status, int32_t _seq,
                 std::vector<tk_command> &_command,
                 std::array<int32_t, GROUP_SZIE> &_deps) :
    AcceptorId(_acceptorId), Replica(_replica), Instance(_instance), Ok(_ok),
    Ballot(_ballot), Seq(_seq), Status(_status), Command(_command), Deps(_deps) {
        Type = PREPARE_REPLY;
    }
};

struct PreAccept {
    TYPE Type;
    int32_t LeaderId;
    int32_t Replica;
    int32_t  Instance;
    int32_t Ballot;
    int32_t  Seq;
    std::vector<tk_command> Command;
    std::array<int32_t, GROUP_SZIE> Deps;
    PreAccept(int32_t _leaderId, int32_t _replica, int32_t _instance,
              int32_t _ballot, int32_t _seq, std::vector<tk_command> &_command,
              std::array<int32_t, GROUP_SZIE> &_deps):
        LeaderId(_leaderId), Replica(_replica), Instance(_instance), Ballot(_ballot),
        Seq(_seq), Command(_command), Deps(_deps){
        Type = PREACCEPT;
    }
};

struct PreAcceptReply {
    bool  Ok;
    TYPE Type;
    int32_t Replica;
    int32_t Instance;
    int32_t  Ballot;
    int32_t  Seq;
    std::array<int32_t, GROUP_SZIE> Deps;
    std::array<int32_t, GROUP_SZIE> CommittedDeps;
    PreAcceptReply(int32_t _replica, int32_t _instance, bool _ok, int32_t _ballot,
                   int32_t _seq, std::array<int32_t, GROUP_SZIE> &_deps,
                   std::array<int32_t, GROUP_SZIE> _cmDeps) :
        Replica(_replica), Instance(_instance), Ok(_ok), Ballot(_ballot),
        Seq(_seq), Deps(_deps), CommittedDeps(_cmDeps) {
        Type = PREACCEPT_REPLY;
    }
};

struct PreAcceptOk {
    TYPE Type;
    int32_t Instance;
    PreAcceptOk(int32_t _instance) : Instance(_instance) {
        Type = PREACCEPT_OK;
    }
};

struct Accept {
    TYPE Type;
    int32_t  LeaderId;
    int32_t  Replica;
    int32_t  Instance;
    int32_t  Ballot;
    int32_t  Count;
    int32_t  Seq;
    std::array<int32_t, GROUP_SZIE> Deps;
    Accept(int32_t _leaderId, int32_t _replica, int32_t _instance,
           int32_t _ballot, int32_t _count, int32_t _seq,
           std::array<int32_t, GROUP_SZIE> & _deps) :
        LeaderId(_leaderId), Replica(_replica), Instance(_instance),
        Ballot(_ballot), Count(_count), Seq(_seq), Deps(_deps) {
        Type = ACCEPT;
    }
};

struct AcceptReply {
    bool Ok;
    TYPE Type;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    AcceptReply(int32_t _replica, int32_t _instance, int32_t _ballot, bool _ok) :
        Replica(_replica), Instance(_instance), Ballot(_ballot), Ok(_ok) {
        Type = ACCEPT_REPLY;
    }
};

struct Commit {
    TYPE Type;
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Seq;
    std::vector<tk_command> Command;
    std::array<int32_t, GROUP_SZIE> Deps;
    Commit(int32_t _leaderId, int32_t _replica, int32_t _instance, int32_t _seq,
        std::vector<tk_command> &_cmds, std::array<int32_t, GROUP_SZIE> &_deps) :
        LeaderId(_leaderId), Replica(_replica), Instance(_instance),
        Seq(_seq), Command(_cmds), Deps(_deps) {
        Type = COMMIT;
    }
};

struct CommitShort {
    TYPE Type;
    int32_t  LeaderId;
    int32_t  Replica;
    int32_t  Instance;
    int32_t  Count;
    int32_t  Seq;
    std::array<int32_t, GROUP_SZIE> Deps;
    CommitShort(int32_t _leaderId, int32_t _replica, int32_t _instance,
                int32_t _count, int32_t _seq, std::array<int32_t, GROUP_SZIE> & _deps):
        LeaderId(_leaderId), Replica(_replica), Instance(_instance), Count(_count),
        Seq(_seq), Deps(_deps) {
        Type = COMMIT_SHORT;
    }
};

struct TryPreAccept {
    TYPE Type;
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    int32_t Seq;
    std::vector<tk_command> Command;
    std::array<int32_t, GROUP_SZIE> Deps;
    TryPreAccept(int32_t _leaderId, int32_t _replica, int32_t _instance, int32_t _ballot,
                 int32_t _seq, std::vector<tk_command> &_cmds, std::array<int32_t, GROUP_SZIE> &_deps) :
        LeaderId(_leaderId), Replica(_replica), Instance(_instance), Ballot(_ballot),
        Seq(_seq), Command(_cmds), Deps(_deps) {
        Type = TRY_PREACCEPT;
    }
};

struct TryPreAcceptReply {
    bool Ok;
    TYPE Type;
    int32_t AcceptorId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    int32_t ConflictReplica;
    int32_t ConflictInstance;
    int32_t ConflictStatus;
    TryPreAcceptReply(int32_t _acceptorId, int32_t _replica, int32_t  _instance, int32_t _ballot, bool _ok,
                      int32_t _conflictReplica, int32_t _conflictInstance, int32_t _conflictStatus) :
        AcceptorId(_acceptorId), Replica(_replica), Instance(_instance), Ballot(_ballot),
        ConflictReplica(_conflictReplica), ConflictInstance(_conflictInstance), ConflictStatus(_conflictStatus) {
        Type = TRY_PREACCEPT_REPLY;
        Ok = _ok;
    }
};

struct Propose {
    TYPE Type;
    int32_t CommandId;
    int64_t Timestamp;
    tk_command Command;
    // TODO: RDMA REPLY HANDLE
    Propose(int32_t _commandId, int64_t _timestamp, tk_command & _cmd) :
        CommandId(_commandId), Timestamp(_timestamp), Command(_cmd) {
        Type = PROPOSE;
    }
};

struct ProposeReply {
    TYPE Type;
    uint8_t OK;
    int32_t CommandId;
    ProposeReply(uint8_t _ok, int32_t _cmdId) : OK(_ok), CommandId(_cmdId) {
        Type = PROPOSE_REPLY;
    }
};

struct InstanceId {
    TYPE Type;
    int32_t replica;
    int32_t instance;
    InstanceId(int32_t _replica, int32_t _instance) :
        replica(_replica), instance(_instance){
        Type = INSTANCE_ID;
    }
};

struct Clock {
    uint32_t Type;
};

struct Beacon_msg {
    uint32_t Type;
    int Rid;
    uint64_t timestamp;
};

struct ClientConnect {
    uint32_t type;
};

#endif //TKDATABASE_TK_MESSAGE_H
