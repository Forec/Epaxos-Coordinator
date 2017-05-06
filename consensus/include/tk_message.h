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
    Prepare() {
        Type = PREPARE;
    }
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
    PrepareReply() {
        Type = PREPARE_REPLY;
    }
    PrepareReply(int32_t _acceptorId, int32_t _replica, int32_t _instance,
                 bool _ok, int32_t _ballot, STATUS _status,
                 std::vector<tk_command> &_command,
                 int32_t _seq,
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
    int32_t Instance;
    int32_t Ballot;
    int32_t  Seq;
    std::vector<tk_command> Command;
    std::array<int32_t, GROUP_SZIE> Deps;
    PreAccept() {
        Type = PREACCEPT;
    }
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
    PreAcceptReply() {
        Type = PREACCEPT_REPLY;
    }
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
    PreAcceptOk() {
        Type = PREACCEPT_OK;
    }
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
    Accept() {
        Type = ACCEPT;
    }
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
    AcceptReply() {
        Type = ACCEPT_REPLY;
    }
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
    Commit() {
        Type = COMMIT;
    }
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
    CommitShort() {
        Type = COMMIT_SHORT;
    }
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
    TryPreAccept() {
        Type = TRY_PREACCEPT;
    }
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
    TryPreAcceptReply() {
        Type = TRY_PREACCEPT_REPLY;
    }
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
    Propose() {
        Type = PROPOSE;
    }
    Propose(int32_t _commandId, int64_t _timestamp, tk_command & _cmd) :
        CommandId(_commandId), Timestamp(_timestamp), Command(_cmd) {
        Type = PROPOSE;
    }
};

struct ProposeReply {
    TYPE Type;
    bool OK;
    int32_t CommandId;
    ProposeReply() {
        Type = PROPOSE_REPLY;
    }
    ProposeReply(bool _ok, int32_t _cmdId) : OK(_ok), CommandId(_cmdId) {
        Type = PROPOSE_REPLY;
    }
};

struct ProposeReplyTS {
    TYPE Type;
    bool OK;
    int32_t CommandId;
    char * Value;
    int64_t Timestamp;
    ProposeReplyTS() {
        Type = PROPOSE_REPLY_TS;
    }
    ProposeReplyTS(bool _ok, int32_t _cmId, char * _v, int64_t _ts):
        OK(_ok), CommandId(_cmId), Value(_v), Timestamp(_ts) {
        Type = PROPOSE_REPLY_TS;
    }
};

struct Read {
    TYPE Type;
    int32_t CommandId;
    std::string Key;
    Read() {
        Type = READ;
    }
    Read(int32_t _cmdId, const std::string & _key) :
        CommandId(_cmdId), Key(_key){
        Type = READ;
    }
};

struct ProposeAndRead {
    TYPE Type;
    int32_t CommandId;
    tk_command Command;
    std::string Key;
    ProposeAndRead() {
        Type = PROPOSE_AND_READ;
    }
    ProposeAndRead(int32_t _cmdId, tk_command & cmd, const std::string & _key):
        CommandId(_cmdId), Command(cmd), Key(_key) {
        Type = PROPOSE_AND_READ;
    }
};

struct InstanceId {
    TYPE Type;
    int32_t replica;
    int32_t instance;
    InstanceId() {
        Type = INSTANCE_ID;
    }
    InstanceId(int32_t _replica, int32_t _instance) :
        replica(_replica), instance(_instance){
        Type = INSTANCE_ID;
    }
};

struct Clock {
    TYPE Type;
    Clock(TYPE _t = SLOW_CLOCK) {
        Type = _t;
    }
};

struct Beacon_msg {
    TYPE Type;
    int Rid;
    uint64_t timestamp;
    Beacon_msg() {
        Type = BEACON;
    }
    Beacon_msg(int _rid, uint64_t _ts) : Rid(_rid), timestamp(_ts){
        Type = BEACON;
    }
};

struct Beacon_msg_reply {
    TYPE Type;
    uint64_t timestamp;
    Beacon_msg_reply() {
        Type = BEACON_REPLY;
    }
    Beacon_msg_reply(uint64_t _ts) : timestamp(_ts){
        Type = BEACON_REPLY;
    }
};

struct ClientConnect {
    TYPE Type;
    ClientConnect() {
        Type = CLIENT_CONNECT;
    }
};

#endif //TKDATABASE_TK_MESSAGE_H
