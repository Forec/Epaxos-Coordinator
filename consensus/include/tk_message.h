//
// Created by jingle on 17-3-31.
//

#ifndef TKDATABASE_TK_MESSAGE_H
#define TKDATABASE_TK_MESSAGE_H

#include "../../config/config.h"
#include "../../utils/include/communication.h"
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *) & LeaderId, 4);
        readUntil(sock, (char *) & Replica, 4);
        readUntil(sock, (char *) & Instance, 4);
        readUntil(sock, (char *) & Ballot, 4);
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *) & msgType, 1);
        sendData(sock, (char *) & LeaderId, 4);
        sendData(sock, (char *) & Replica, 4);
        sendData(sock, (char *) & Instance, 4);
        sendData(sock, (char *) & Ballot, 4);
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
    bool Unmarshal(int sock) {
        uint8_t tmp;
        readUntil(sock, (char *)&tmp, 1);
        Ok = (bool)tmp;
        readUntil(sock, (char *)&tmp, 1);
        Status = (STATUS)tmp;
        readUntil(sock, (char *) &AcceptorId, 4);
        readUntil(sock, (char *) &Replica, 4);
        readUntil(sock, (char *) &Instance, 4);
        readUntil(sock, (char *) &Ballot, 4);
        readUntil(sock, (char *) &Seq, 4);
        uint32_t tmp32;
        readUntil(sock, (char *) &tmp32, 4);
        Command.resize(tmp32, tk_command());
        for (int i = 0; i < tmp32; i++) {
            if (!Command[i].Unmarshal(sock))
                return false;
        }
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *) &tmp32, 4);
            Deps[i] = tmp32;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *) &msgType, 1);
        msgType = (uint8_t )Ok;
        sendData(sock, (char *) &msgType, 1);
        msgType = (uint8_t )Status;
        sendData(sock, (char *) &msgType, 1);
        sendData(sock, (char *) &AcceptorId, 4);
        sendData(sock, (char *) &Replica, 4);
        sendData(sock, (char *) &Instance, 4);
        sendData(sock, (char *) &Ballot, 4);
        sendData(sock, (char *) &Seq, 4);
        int32_t tmp32 = (int32_t)Command.size();
        sendData(sock, (char *) &tmp32, 4);
        for (int i = 0; i < Command.size(); i++) {
            Command[i].Marshal(sock);
        }
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = Deps[i];
            sendData(sock, (char *) &tmp32, 4);
        }
    }
};

struct PreAccept {
    TYPE Type;
    int32_t LeaderId;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    int32_t Seq;
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *)&LeaderId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Ballot, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t commandSize;
        readUntil(sock, (char *)&commandSize, 4);
        Command.resize((unsigned long)commandSize, tk_command());
        for (int i = 0; i < commandSize; i++) {
            Command[i].Unmarshal(sock);
        }
        int32_t tmp;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *)&tmp, 4);
            Deps[i] = tmp;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Ballot, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t commandSize = (int32_t) Command.size();
        sendData(sock, (char *)&commandSize, 4);
        for (int i = 0; i < commandSize; i++)
            Command[i].Marshal(sock);
        for (int i = 0; i < GROUP_SZIE; i++) {
            int32_t tmp = Deps[i];
            sendData(sock, (char *)&tmp, 4);
        }
    }
};

struct PreAcceptReply {
    bool  Ok;
    TYPE Type;
    int32_t Replica;
    int32_t Instance;
    int32_t Ballot;
    int32_t Seq;
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
    bool Unmarshal(int sock) {
        uint8_t tmp;
        readUntil(sock, (char *)&tmp, 1);
        Ok = tmp;
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Ballot, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *) &tmp32, 4);
            Deps[i] = tmp32;
        }
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *) &tmp32, 4);
            CommittedDeps[i] = tmp32;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t tmp = (uint8_t )Type;
        sendData(sock, (char *)&tmp, 1);
        tmp = (uint8_t )Ok;
        sendData(sock, (char *)&tmp, 1);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Ballot, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = Deps[i];
            sendData(sock, (char *)&tmp32, 4);
        }
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = CommittedDeps[i];
            sendData(sock, (char *)&tmp32, 4);
        }
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
    bool Unmarshal(int sock) {
        return readUntil(sock, (char *) &Instance, 4) == 0;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&Instance, 4);
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *)&LeaderId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Ballot, 4);
        readUntil(sock, (char *)&Count, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *)&tmp32, 4);
            Deps[i] = tmp32;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Ballot, 4);
        sendData(sock, (char *)&Count, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = Deps[i];
            sendData(sock, (char *)&tmp32, 4);
        }
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
    bool Unmarshal(int sock) {
        uint8_t tmp;
        readUntil(sock, (char *) &tmp, 1);
        Ok = (bool)tmp;
        readUntil(sock, (char *) &Replica, 4);
        readUntil(sock, (char *) &Instance, 4);
        readUntil(sock, (char *) &Ballot, 4);
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *) &msgType, 1);
        msgType = (uint8_t )Ok;
        sendData(sock, (char *) &msgType, 1);
        sendData(sock, (char *) &Replica, 4);
        sendData(sock, (char *) &Instance, 4);
        sendData(sock, (char *) &Ballot, 4);
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *)&LeaderId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t commandSize;
        readUntil(sock, (char *)&commandSize, 4);
        Command.resize((unsigned long)commandSize, tk_command());
        for (int i = 0; i < commandSize; i++) {
            Command[i].Unmarshal(sock);
        }
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *)&tmp32, 4);
            Deps[i] = tmp32;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t commandSize = (int32_t) Command.size();
        sendData(sock, (char *)&commandSize, 4);
        for (int i = 0; i < commandSize; i++)
            Command[i].Marshal(sock);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = Deps[i];
            sendData(sock, (char *)&tmp32, 4);
        }
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *)&LeaderId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Count, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *)&tmp32, 4);
            Deps[i] = tmp32;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Count, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t tmp32;
        for (int i = 0; i < GROUP_SZIE; i++) {
            tmp32 = Deps[i];
            sendData(sock, (char *)&tmp32, 4);
        }
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
    bool Unmarshal(int sock) {
        readUntil(sock, (char *)&LeaderId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Ballot, 4);
        readUntil(sock, (char *)&Seq, 4);
        int32_t commandSize;
        readUntil(sock, (char *)&commandSize, 4);
        Command.resize((unsigned long)commandSize, tk_command());
        for (int i = 0; i < commandSize; i++) {
            Command[i].Unmarshal(sock);
        }
        int32_t tmp;
        for (int i = 0; i < GROUP_SZIE; i++) {
            readUntil(sock, (char *)&tmp, 4);
            Deps[i] = tmp;
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Ballot, 4);
        sendData(sock, (char *)&Seq, 4);
        int32_t commandSize = (int32_t) Command.size();
        sendData(sock, (char *)&commandSize, 4);
        for (int i = 0; i < commandSize; i++)
            Command[i].Marshal(sock);
        for (int i = 0; i < GROUP_SZIE; i++) {
            int32_t tmp = Deps[i];
            sendData(sock, (char *)&tmp, 4);
        }
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
    bool Unmarshal(int sock) {
        uint8_t tmp;
        readUntil(sock, (char *)&tmp, 1);
        Ok = (bool)tmp;
        readUntil(sock, (char *)&AcceptorId, 4);
        readUntil(sock, (char *)&Replica, 4);
        readUntil(sock, (char *)&Instance, 4);
        readUntil(sock, (char *)&Ballot, 4);
        readUntil(sock, (char *)&ConflictReplica, 4);
        readUntil(sock, (char *)&ConflictInstance, 4);
        readUntil(sock, (char *)&ConflictStatus, 4);
        return true;
    }
    void Marshal(int sock) {
        uint8_t tmp = (uint8_t) Type;
        sendData(sock, (char *)&tmp, 1);
        tmp = (uint8_t) Ok;
        sendData(sock, (char *)&tmp, 1);
        sendData(sock, (char *)&AcceptorId, 4);
        sendData(sock, (char *)&Replica, 4);
        sendData(sock, (char *)&Instance, 4);
        sendData(sock, (char *)&Ballot, 4);
        sendData(sock, (char *)&ConflictReplica, 4);
        sendData(sock, (char *)&ConflictInstance, 4);
        sendData(sock, (char *)&ConflictStatus, 4);
    }
};

struct Propose {
    TYPE Type;
    int32_t CommandId;
    int64_t Timestamp;
    tk_command Command;
    // TODO - DONE
    RDMA_CONNECTION Conn;
    Propose() {
        Type = PROPOSE;
    }
    Propose(int32_t _commandId, int64_t _timestamp, tk_command & _cmd, RDMA_CONNECTION _conn) :
        CommandId(_commandId), Timestamp(_timestamp), Command(_cmd), Conn(_conn) {
        Type = PROPOSE;
    }
    bool Unmarshal(int sock) {
        char buf[8];
        if (readUntil(sock, buf, 4) < 0)
            return false;
        CommandId = *(int32_t *) buf;
        if (readUntil(sock, buf, 8) < 0)
            return false;
        Timestamp = *(int64_t *) buf;
        if (readUntil(sock, buf, 4) < 0)
            return false;
        Conn = *(RDMA_CONNECTION *) buf;
        return Command.Unmarshal(sock);
    }
    void Marshal(int sock) {
        char buf[21];
        buf[0] = (char) Type;
        memcpy(buf + 1, &CommandId, 4);
        memcpy(buf + 5, &Timestamp, 8);
        memcpy(buf + 13, &Conn, 4);
        sendData(sock, buf, 17);
        Command.Marshal(sock);
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
    int32_t ValueSize;
    char * Value;
    int64_t Timestamp;
    ProposeReplyTS() {
        Type = PROPOSE_REPLY_TS;
        Value = nullptr;
    }
    ProposeReplyTS(bool _ok, int32_t _cmId, int32_t _vs, char * _v, int64_t _ts):
        OK(_ok), CommandId(_cmId), ValueSize(_vs), Value(_v), Timestamp(_ts) {
        Type = PROPOSE_REPLY_TS;
    }
    bool Unmarshal(int sock) {
        uint8_t tmp;
        readUntil(sock, (char *) &tmp, 1);
        OK = (bool) tmp;
        readUntil(sock, (char *) &CommandId, 4);
        readUntil(sock, (char *) &ValueSize, 4);
        readUntil(sock, (char *) &Timestamp, 8);
        if (Value)
            delete [] Value;
        Value = new char[ValueSize];
        return readUntil(sock, Value, ValueSize) == 0;
    }
    void Marshal(int sock) {
        uint8_t tmp = (uint8_t) Type;
        sendData(sock, (char *)&tmp, 1);
        tmp = (uint8_t) OK;
        sendData(sock, (char *)&tmp, 1);
        sendData(sock, (char *)&CommandId, 4);
        sendData(sock, (char *)&ValueSize, 4);
        sendData(sock, (char *)&Timestamp, 8);
        sendData(sock, Value, ValueSize);
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
    bool Unmarshal(int sock) {
        char buf[8];
        if (readUntil(sock, buf, 4) < 0)
            return false;
        Rid = *(int *) buf;
        if (readUntil(sock, buf, 8) < 0)
            return false;
        timestamp = *(uint64_t *) buf;
        return true;
    }
    void Marshal(int sock) {
        char buf[16];
        buf[0] = (char) Type;
        memcpy(buf + 1, &Rid, 4);
        memcpy(buf + 5, &timestamp, 8);
        sendData(sock, buf, 13);
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
    bool Unmarshal(int sock) {
        char buf[8];
        if (readUntil(sock, buf, 8) < 0)
            return false;
        timestamp = *(uint64_t *) buf;
        return true;
    }
    void Marshal(int sock) {
        char buf[10];
        buf[0] = (char) Type;
        memcpy(buf + 1, &timestamp, 8);
        sendData(sock, buf, 9);
    }
};

struct ClientConnect {
    TYPE Type;
    ClientConnect() {
        Type = CLIENT_CONNECT;
    }
};

struct RegisterArgs {
    TYPE Type;
    std::string Addr;
    int Port;
    RegisterArgs() {
        Type = REGISTER_ARGS;
    }
    RegisterArgs(std::string addr, int port) :
        Addr(addr), Port(port){
        Type = REGISTER_ARGS;
    }
    bool Unmarshal(int sock) {
        int32_t tmp32;
        if (readUntil(sock, (char *) & tmp32, 4) != 0)
            return false;
        char * buf = new char[tmp32];
        if (readUntil(sock, buf, tmp32) != 0)
            return false;
        Addr = std::string(buf);
        if (readUntil(sock, (char *) & Port, 4) != 0)
            return false;
        delete buf;
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *) &msgType, 1);
        int32_t size = Addr.length() + 1;
        sendData(sock, (char *) &size, 4);
        char * buf = new char[size];
        buf[size] = '\0';
        strcpy(buf, Addr.c_str());
        sendData(sock, buf, size);
        sendData(sock, (char *)&Port, 4);
    }
};

struct RegisterReply {
    TYPE Type;
    int ReplicaId;
    std::vector<std::string> AddrList;
    std::vector<int> PortList;
    bool Ready;
    RegisterReply() {
        Type = REGISTER_REPLY;
    }
    RegisterReply(int _r, bool _ready, std::vector<std::string> & addrList, std::vector<int> &portList):
        ReplicaId(_r), Ready(_ready), AddrList(addrList), PortList(portList) {
        Type = REGISTER_REPLY;
    }
    bool Unmarshal(int sock) {
        uint8_t tmp;
        if (readUntil(sock, (char *) & tmp, 1) != 0)
            return false;
        Ready = (bool) tmp;
        int32_t tmp32;
        if (readUntil(sock, (char *) & tmp32, 4) != 0)
            return false;
        ReplicaId = tmp32;
        char buf[64];
        if (readUntil(sock, (char *) & tmp32, 4) != 0)
            return false;
        int32_t size = tmp32;
        AddrList.clear();
        for (int i = 0; i < size; i++) {
            if (readUntil(sock, (char *) & tmp32, 4) != 0)
                return false;
            if (readUntil(sock, buf, tmp32) != 0)
                return false;
            buf[tmp32 - 1] = '\0';
            AddrList.push_back(std::string(buf));
        }
        if (readUntil(sock, (char *)&size, 4) != 0)
            return false;
        PortList.clear();
        for (int i = 0; i < size; i++) {
            if (readUntil(sock, (char *) & tmp32, 4) != 0)
                return false;
            PortList.push_back(tmp32);
        }
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *) &msgType, 1);
        msgType = (uint8_t) Ready;
        sendData(sock, (char *) &msgType, 1);
        sendData(sock, (char *) &ReplicaId, 4);
        int32_t size = AddrList.size();
        sendData(sock, (char *) &size, 4);
        char buf[64];
        for (int i = 0; i < AddrList.size(); i++) {
            size = AddrList[i].length() + 1;
            sendData(sock, (char *) &size, 4);
            buf[size - 1] = '\0';
            strcpy(buf, AddrList[i].c_str());
            sendData(sock, buf, size);
        }
        size = PortList.size();
        sendData(sock, (char *) &size, 4);
        for (int i = 0; i < PortList.size(); i++) {
            size = PortList[i];
            sendData(sock, (char *) &size, 4);
        }
    }
};

struct GetLeaderArgs {
    TYPE Type;
    GetLeaderArgs() {
        Type = GET_LEADER_ARGS;
    }
    bool Unmarshal(int sock) {
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
    }
};

struct GetLeaderReply {
    TYPE Type;
    int32_t LeaderId;
    GetLeaderReply() {
        Type = GET_LEADER_REPLY;
    }
    GetLeaderReply(int32_t _l) : LeaderId(_l) {
        Type = GET_LEADER_REPLY;
    }
    bool Unmarshal(int sock) {
        return readUntil(sock, (char *)&LeaderId, 4) == 0;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        sendData(sock, (char *)&LeaderId, 4);
    }
};

struct GetReplicaListArgs {
    TYPE Type;
    GetReplicaListArgs() {
        Type = GET_REPLICA_LIST_ARGS;
    }
    bool Unmarshal(int sock) {
        return true;
    }
    void Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
    }
};

struct GetReplicaListReply {
    TYPE Type;
    bool Ready;
    std::vector<std::string> ReplicaAddrList;
    std::vector<int> ReplicaPortList;
    GetReplicaListReply() {
        Type = GET_REPLICA_LIST_REPLY;
    }
    GetReplicaListReply(bool _r, std::vector<std::string> &addr, std::vector<int> & port) :
        Ready(_r), ReplicaAddrList(addr), ReplicaPortList(port){
        Type = GET_REPLICA_LIST_REPLY;
    }
    bool Unmarshal(int sock) {
        uint8_t tmp;
        if (readUntil(sock, (char *) & tmp, 1) != 0)
            return false;
        Ready = (bool) tmp;
        int32_t tmp32;
        char buf[64];
        if (readUntil(sock, (char *) & tmp32, 4) != 0)
            return false;
        int32_t size = tmp32;
        ReplicaAddrList.clear();
        for (int i = 0; i < size; i++) {
            if (readUntil(sock, (char *) & tmp32, 4) != 0)
                return false;
            if (readUntil(sock, buf, tmp32) != 0)
                return false;
            buf[tmp32 - 1] = '\0';
            ReplicaAddrList.push_back(std::string(buf));
        }
        if (readUntil(sock, (char *)&size, 4) != 0)
            return false;
        ReplicaPortList.clear();
        for (int i = 0; i < size; i++) {
            if (readUntil(sock, (char *) & tmp32, 4) != 0)
                return false;
            ReplicaPortList.push_back(tmp32);
        }
        return true;
    }
    bool Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        sendData(sock, (char *)&msgType, 1);
        msgType = (uint8_t) Ready;
        sendData(sock, (char *)&msgType, 1);
        int32_t size = ReplicaAddrList.size();
        sendData(sock, (char *) &size, 4);
        char buf[64];
        for (int i = 0; i < ReplicaAddrList.size(); i++) {
            size = ReplicaAddrList[i].length() + 1;
            sendData(sock, (char *) &size, 4);
            buf[size - 1] = '\0';
            strcpy(buf, ReplicaAddrList[i].c_str());
            sendData(sock, buf, size);
        }
        size = ReplicaPortList.size();
        sendData(sock, (char *) &size, 4);
        for (int i = 0; i < ReplicaPortList.size(); i++) {
            size = ReplicaPortList[i];
            sendData(sock, (char *) &size, 4);
        }
        return true;
    }
};


struct BeTheLeaderReply {
    TYPE Type;
    bool Ok;
    BeTheLeaderReply() {
        Type = BE_LEADER_REPLY;
    }
    BeTheLeaderReply(bool _ok) : Ok(_ok) {
        Type = BE_LEADER_REPLY;
    }
    bool Unmarshal(int sock) {
        uint8_t tmp;
        if (readUntil(sock, (char *) & tmp, 1) != 0)
            return false;
        Ok = (bool) tmp;
        return true;
    }
    bool Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        if (sendData(sock, (char *)&msgType, 1) != 1)
            return false;
        msgType = (uint8_t) Ok;
        if (sendData(sock, (char *)&msgType, 1) != 1)
            return false;
    }
};

struct GENERAL {
    TYPE Type;
    GENERAL(TYPE _t): Type(_t) {
    }
    bool Unmarshal(int sock) {
        uint8_t msgType;
        if (readUntil(sock, (char *)&msgType, 1) != 0)
            return false;
        if ((TYPE)msgType == Type)
            return true;
        return false;
    }
    bool Marshal(int sock) {
        uint8_t msgType = (uint8_t) Type;
        return sendData(sock, (char *)&msgType, 1) == 1;
    }
};

#endif //TKDATABASE_TK_MESSAGE_H
