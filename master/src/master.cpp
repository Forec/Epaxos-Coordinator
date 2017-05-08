//
// Created by forec on 17-5-8.
//

#include "../include/master.h"
#include "../../utils/include/utils.h"
#include <stdio.h>
#include <gflags/gflags.h> // libgflags-dev    LBIS += lgflags

static bool ValidatePort(const char* flagname, int32_t value) {
    if (value > 0 && value < 32768)
        return true;
    printf("Invalid value for --%s: %d\n", flagname, (int)value);
    return false;
}

static bool ValidateN(const char* flagname, int32_t value) {
    if (value >= 3 && value <= 13 && value % 2 == 1)
        return true;
    printf("Invalid value for --%s: %d\n", flagname, (int)value);
    return false;
}

DEFINE_int32(port, 7087, "Port # to listen on. Defaults to 7087");
DEFINE_validator(port, &ValidatePort);
DEFINE_int32(N, GROUP_SZIE, "Number of replicas. Defaults to 3");
DEFINE_validator(N, &ValidateN);

int main(int argc, char * argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    fprintf(stdout, "Master starting on port %d\n", FLAGS_port);
    fprintf(stdout, "...waiting for %d replicas\n", FLAGS_N);

    Master * master = new Master();
    master->N = FLAGS_N;
    master->nodeList.clear();
    master->addrList.resize(FLAGS_N, "");
    master->portList.resize(FLAGS_N, 0);
    master->nodes.resize(FLAGS_N, -1);
    master->leader.resize(FLAGS_N, false);
    master->alive.resize(FLAGS_N, false);

    master->Listener = listenOn(FLAGS_port);
    if (master->Listener < 0) {
        fprintf(stderr, "Master listen error!\n");
        exit(1);
    }

    std::thread wc(waitingConnections, master);
    wc.detach();

    for (;;) {
        master->lock.lock();
        if (master->nodeList.size() == master->N) {
            master->lock.unlock();
            break;
        }
        master->lock.unlock();
        nano_sleep(100000000);
    }
    nano_sleep(2000000000);

    // connect to SMR servers
    for (int i = 0; i < master->N; i++) {
        master->nodes[i] = dialTo(master->addrList[i], master->portList[i] + 1000);
        if (master->nodes[i] < 0) {
            fprintf(stderr, "Error connecting to replica %d\n", i);
        }
        master->leader[i] = false;
    }
    master->leader[0] = true;

    for (;;) {
        nano_sleep(3000 * 1000 * 1000);
        bool new_leader = false;
        for (int i = 0; i < master->nodes.size(); i++) {
            if (!ping(master->nodes[i])) {
                master->alive[i] = false;
                if (master->leader[i]) {
                    new_leader = true;
                    master->leader[i] = false;
                }
            } else {
                master->alive[i] = true;
            }
        }
        if (!new_leader) {
            continue;
        }
        for (int i = 0; i < master->nodes.size(); i++) {
            if (master->alive[i]) {
                if (BeTheLeader(master->nodes[i])) {
                    master->leader[i] = true;
                    fprintf(stdout, "Replica %d is the new leader\n", i);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < master->nodes.size(); i++) {
        if (master->nodes[i] != -1)
            destroyConnection(master->nodes[i]);
    }
    return 0;
}

void waitingConnections(Master * master) {
    std::vector<std::thread *> hds;
    for (;;) {
        int nsock = acceptAt(master->Listener);
        if (nsock < 0) {
            continue;
        }
        hds.push_back(new std::thread (handlers, master, nsock));
    }
    for (int i = 0; i < hds.size(); i++) {
        hds[i]->detach();
        delete hds[i];
    }
}

void handlers(Master * master, int nsock) {
    uint8_t msgType;
    for (;;) {
        readUntil(nsock, (char *)&msgType, 1);
        switch ((TYPE) msgType) {
            case REGISTER_ARGS:
                handleRegister(master, nsock);
                break;
            case GET_LEADER_ARGS:
                handleLeaderArgs(master, nsock);
                break;
            case GET_REPLICA_LIST_ARGS:
                handleReplicaListArgs(master, nsock);
                break;
            default:
                fprintf(stdout, "Unknown message type received.\n");
                break;
        }
    }
}

void handleRegister(Master * master, int nsock) {
    RegisterArgs arg;
    if (!arg.Unmarshal(nsock)) {
        fprintf(stdout, "Cannot unmarshal RegisterArgs at socket %d\n", nsock);
        return;
    }

    master->lock.lock();
    int nlen = master->nodeList.size();
    int index = nlen;
    for (int i = 0; i < master->addrList.size(); i++) {
        if (master->addrList[i] == arg.Addr && master->portList[i] == arg.Port) {
            index = i;
            break;
        }
    }
    if (index == nlen) {
        master->addrList.push_back(arg.Addr);
        master->portList.push_back(arg.Port);
        master->nodeList.push_back(arg.Addr + ":" + std::to_string(arg.Port));
        nlen++;
    }

    RegisterReply reply;
    if (nlen == master->N) {
        reply.Ready = true;
        reply.ReplicaId = index;
        reply.AddrList = master->addrList;
        reply.PortList = master->portList;
    } else {
        reply.Ready = false;
    }
    master->lock.unlock();
    reply.Marshal(nsock);
}

void handleLeaderArgs(Master * master, int nsock) {
    std::this_thread::sleep_for(std::chrono::milliseconds(4));  // why?
    for (int i  = 0; i < master->leader.size(); i++) {
        if (master->leader[i]) {
            GetLeaderReply rp(i);
            rp.Marshal(nsock);
            break;
        }
    }
}

void handleReplicaListArgs(Master * master, int nsock) {
    master->lock.lock();
    GetReplicaListReply rp;
    if (master->nodeList.size() == master->N) {
        rp.Ready = true;
        rp.ReplicaAddrList = master->addrList;
        rp.ReplicaPortList = master->portList;
    } else {
        rp.Ready = false;
    }
    master->lock.unlock();
    rp.Marshal(nsock);
}

bool ping(int sock) {
    GENERAL p(PING);
    p.Marshal(sock);
    GENERAL pr(PING_REPLY);
    return pr.Unmarshal(sock);
}

bool BeTheLeader(int sock) {
    GENERAL p(BE_LEADER);
    p.Marshal(sock);
    BeTheLeaderReply br;
    return br.Unmarshal(sock) && br.Ok;
}