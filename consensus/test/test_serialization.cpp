//
// Created by forec on 17-5-9.
//

#include "../../config/config.h"
#include "../../utils/include/communication.h"
#include "../include/tk_message.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <array>
#include <string>

#define TEST_PORT 11091

std::atomic<int> passed(0);
std::atomic<int> failed(0);

void alert(bool conf);
void replyThread();

char val1[16] = "HELLOEPAXOS"; // 11 chars, valSize = 12
char val2[16] = "HELLOWORLD";  // 10 chars, valSize = 11
char val3[16] = "CRAZYCODE";   // 9  chars, valSize = 10

tk_command cmd1(PUT, "test1", 12, val1);
tk_command cmd2(GET, "test2", 11, val2);
tk_command cmd3(GET, "test3", 10, val3);

std::array<int32_t, GROUP_SZIE> test_deps;
std::vector<tk_command> test_cmds = {cmd1, cmd2, cmd3};
std::vector<std::string> addr_list = {"192.168.1.1", "172.16.0.1", "10.0.0.1"};
std::vector<int> port_list;

int main() {

    for (int i = 0; i < GROUP_SZIE; i++) {
        test_deps[i] = i + 1;
        port_list.push_back(i+1);
        if (i >= 3) {
            addr_list.push_back("10.0.0.1");
            test_cmds.push_back(cmd3);
        }
    }

    int sock = listenOn(TEST_PORT);
    alert(sock >= 0);
    if (sock < 0) {
        fprintf(stderr, "Cannot listen on port %d!\n", TEST_PORT);
        exit(1);
    }
    std::thread rpth(replyThread);

    int client = acceptAt(sock);
    alert(client >= 0);
    if (client < 0) {
        fprintf(stderr, "Cannot build connection with test client!\n");
        exit(1);
    }

    uint8_t msgType;

    // send messages to simulate client thread
    Prepare prp(1, 3, 5, 7);
    prp.Marshal(client);
    PrepareReply prpr(1, 3, 5, false, 7, COMMITTED, test_cmds, 9, test_deps);
    prpr.Marshal(client);
    PreAccept pac(1, 3, 5, 7, 9, test_cmds, test_deps);
    pac.Marshal(client);
    PreAcceptOk pacok(13);
    pacok.Marshal(client);
    PreAcceptReply pacrp(1, 3, true, 5, 7, test_deps, test_deps);
    pacrp.Marshal(client);
    Accept ac(1, 3, 5, 7, 9, 11, test_deps);
    ac.Marshal(client);
    AcceptReply acrp(1, 3, 5, true);
    acrp.Marshal(client);
    Commit cm(1, 3, 5, 7, test_cmds, test_deps);
    cm.Marshal(client);

    // receive messages from simulate client thread
    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == COMMIT_SHORT);
    CommitShort cms;
    alert(cms.Unmarshal(client));
    alert(cms.LeaderId == 2);
    alert(cms.Replica == 4);
    alert(cms.Instance == 6);
    alert(cms.Count == 8);
    alert(cms.Seq == 10);
    alert(cms.Deps.size() == GROUP_SZIE);
    alert(cms.Deps[GROUP_SZIE - 1] == GROUP_SZIE);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == TRY_PREACCEPT);
    TryPreAccept tpac;
    alert(tpac.Unmarshal(client));
    alert(tpac.LeaderId == 2);
    alert(tpac.Replica == 4);
    alert(tpac.Instance == 6);
    alert(tpac.Ballot == 8);
    alert(tpac.Seq == 10);
    alert(tpac.Command.size() == GROUP_SZIE);
    alert(tpac.Command[0].key == "test1");
    alert(tpac.Deps.size() == GROUP_SZIE);
    alert(tpac.Deps[GROUP_SZIE - 1] == GROUP_SZIE);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == TRY_PREACCEPT_REPLY);
    TryPreAcceptReply tpacr;
    alert(tpacr.Unmarshal(client));
    alert(tpacr.AcceptorId == 2);
    alert(tpacr.Replica == 4);
    alert(tpacr.Instance == 6);
    alert(tpacr.Ballot == 8);
    alert(!tpacr.Ok);
    alert(tpacr.ConflictReplica == 10);
    alert(tpacr.ConflictInstance == 12);
    alert(tpacr.ConflictStatus == 14);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == PROPOSE);
    Propose pr;
    alert(pr.Unmarshal(client));
    alert(pr.CommandId == 2);
    alert(pr.Timestamp == 4);
    alert(pr.Conn == 6);
    alert(pr.Command.opcode == PUT);
    alert(pr.Command.key == "test1");
    alert(pr.Command.valSize == 12);
    alert(!strcmp(pr.Command.val, val1));

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == PROPOSE_REPLY_TS);
    ProposeReplyTS prprp;
    alert(prprp.Unmarshal(client));
    alert(prprp.OK);
    alert(prprp.CommandId == 2);
    alert(prprp.ValueSize == 12);
    alert(!strcmp(prprp.Value, val1));
    alert(prprp.Timestamp == 6);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == BEACON);
    Beacon_msg bcm;
    alert(bcm.Unmarshal(client));
    alert(bcm.Rid == 2);
    alert(bcm.timestamp == 4);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == BEACON_REPLY);
    Beacon_msg_reply bcmr;
    alert(bcmr.Unmarshal(client));
    alert(bcmr.timestamp == 102);

    // entering interactive mode
    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == REGISTER_ARGS);
    RegisterArgs rgar;
    alert(rgar.Unmarshal(client));
    alert(rgar.Addr == "192.168.0.1");
    alert(rgar.Port == 10001);
    RegisterReply rgrp(1, true, addr_list, port_list);
    rgrp.Marshal(client);

    GetReplicaListArgs grlas;
    grlas.Marshal(client);
    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == GET_REPLICA_LIST_REPLY);
    GetReplicaListReply grlrp;
    alert(grlrp.Unmarshal(client));
    alert(grlrp.Ready);
    alert(grlrp.ReplicaPortList.size() == GROUP_SZIE);
    alert(grlrp.ReplicaAddrList.size() == GROUP_SZIE);
    alert(grlrp.ReplicaPortList[GROUP_SZIE - 1] == GROUP_SZIE);
    alert(grlrp.ReplicaAddrList[GROUP_SZIE - 1] == "10.0.0.1");

    GENERAL btl(BE_LEADER);
    btl.Marshal(client);

    alert(readUntil(client, (char *) &msgType, 1) == 0);
    alert((TYPE) msgType == BE_LEADER_REPLY);
    BeTheLeaderReply btlr;
    alert(btlr.Unmarshal(client));
    alert(!btlr.Ok);

    destroyConnection(client);
    destroyConnection(sock);
    rpth.detach();

    fprintf(stdout, "====================== TEST SERIALIZATION ======================\n");
    fprintf(stdout, "                  passsed: %d, failed: %d\n", passed.load(), failed.load());
    return 0;
}

void replyThread() {
    int sock = dialTo("127.0.0.1", TEST_PORT);
    alert(sock >= 0);
    if (sock < 0) {
        fprintf(stderr, "Cannot connect to 127.0.0.1:%d!\n", TEST_PORT);
        exit(1);
    }
    uint8_t msgType;

    // receive messages from server thread
    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == PREPARE);
    Prepare prp;
    alert(prp.Unmarshal(sock));
    alert(prp.LeaderId == 1);
    alert(prp.Replica == 3);
    alert(prp.Instance == 5);
    alert(prp.Ballot == 7);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == PREPARE_REPLY);
    PrepareReply prpr;
    alert(prpr.Unmarshal(sock));
    alert(prpr.AcceptorId == 1);
    alert(prpr.Replica == 3);
    alert(prpr.Instance == 5);
    alert(prpr.Ballot == 7);
    alert(prpr.Seq == 9);
    alert(!prpr.Ok);
    alert(prpr.Status == COMMITTED);
    alert(prpr.Command.size() == GROUP_SZIE);
    alert(prpr.Command[1].key == "test2");
    alert(prpr.Command[1].valSize == 11);
    alert(prpr.Deps.size() == GROUP_SZIE);
    alert(prpr.Deps[GROUP_SZIE - 1] == GROUP_SZIE);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == PREACCEPT);
    PreAccept pac;
    alert(pac.Unmarshal(sock));
    alert(pac.LeaderId == 1);
    alert(pac.Replica == 3);
    alert(pac.Instance == 5);
    alert(pac.Ballot == 7);
    alert(pac.Seq == 9);
    alert(pac.Command.size() == GROUP_SZIE);
    alert(pac.Command[2].key == "test3");
    alert(pac.Command[2].valSize == 10);
    alert(pac.Deps.size() == GROUP_SZIE);
    alert(pac.Deps[0] == 1);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == PREACCEPT_OK);
    PreAcceptOk pacok;
    alert(pacok.Unmarshal(sock));
    alert(pacok.Instance == 13);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == PREACCEPT_REPLY);
    PreAcceptReply pacrp;
    alert(pacrp.Unmarshal(sock));
    alert(pacrp.Replica == 1);
    alert(pacrp.Instance == 3);
    alert(pacrp.Ballot == 5);
    alert(pacrp.Seq == 7);
    alert(pacrp.Ok);
    alert(pacrp.Deps.size() == GROUP_SZIE);
    alert(pacrp.Deps[0] == 1);
    alert(pacrp.CommittedDeps.size() == GROUP_SZIE);
    alert(pacrp.CommittedDeps[GROUP_SZIE - 1] == GROUP_SZIE);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == ACCEPT);
    Accept ac;
    alert(ac.Unmarshal(sock));
    alert(ac.LeaderId == 1);
    alert(ac.Replica == 3);
    alert(ac.Instance == 5);
    alert(ac.Ballot == 7);
    alert(ac.Count == 9);
    alert(ac.Seq == 11);
    alert(ac.Deps.size() == GROUP_SZIE);
    alert(ac.Deps[1] == 2);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == ACCEPT_REPLY);
    AcceptReply acrp;
    alert(acrp.Unmarshal(sock));
    alert(acrp.Replica == 1);
    alert(acrp.Instance == 3);
    alert(acrp.Ballot == 5);
    alert(acrp.Ok);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == COMMIT);
    Commit cm;
    alert(cm.Unmarshal(sock));
    alert(cm.LeaderId == 1);
    alert(cm.Replica == 3);
    alert(cm.Instance == 5);
    alert(cm.Seq == 7);
    alert(cm.Command.size() == GROUP_SZIE);
    alert(!strcmp(cm.Command[2].val, val3));
    alert(cm.Deps.size() == GROUP_SZIE);
    alert(cm.Deps[2] == 3);

    // send messages to server thread
    CommitShort cms(2, 4, 6, 8, 10, test_deps);
    cms.Marshal(sock);
    TryPreAccept tpac(2, 4, 6, 8, 10, test_cmds, test_deps);
    tpac.Marshal(sock);
    TryPreAcceptReply tpacr(2, 4, 6, 8, false, 10, 12, 14);
    tpacr.Marshal(sock);
    Propose pr(2, 4, cmd1, 6);
    pr.Marshal(sock);
    ProposeReplyTS prprp(true, 2, 12, val1, 6);
    prprp.Marshal(sock);
    Beacon_msg bcm(2, 4);
    bcm.Marshal(sock);
    Beacon_msg_reply bcmr(102);
    bcmr.Marshal(sock);

    // entering interactive mode
    RegisterArgs rgar("192.168.0.1", 10001);
    rgar.Marshal(sock);
    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == REGISTER_REPLY);
    RegisterReply rgrp;
    alert(rgrp.Unmarshal(sock));
    alert(rgrp.Ready);
    alert(rgrp.ReplicaId == 1);
    alert(rgrp.AddrList.size() == GROUP_SZIE);
    alert(rgrp.PortList.size() == GROUP_SZIE);
    alert(rgrp.PortList[0] == 1);
    alert(rgrp.AddrList[0] == "192.168.1.1");

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == GET_REPLICA_LIST_ARGS);
    GetReplicaListArgs grlas;
    alert(grlas.Unmarshal(sock));
    GetReplicaListReply grlrp(true, addr_list, port_list);
    grlrp.Marshal(sock);

    alert(readUntil(sock, (char *)&msgType, 1) == 0);
    alert((TYPE) msgType == BE_LEADER);
    BeTheLeaderReply btlr(false);
    btlr.Marshal(sock);

    destroyConnection(sock);
    return;
}

void alert(bool conf) {
    if (conf)
        passed++;
    else
        failed++;
    if (!conf) {
        printf("failed once!\n");
    }
}
