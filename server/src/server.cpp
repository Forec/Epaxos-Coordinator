//
// Created by forec on 17-5-8.
//

#include "../include/server.h"
#include <gflags/gflags.h>
#include <stdio.h>
#include "../../utils/include/utils.h"

DEFINE_string(maddr, "127.0.0.1", "Master address, default to localhost.");
DEFINE_int32(mport, MASTER_PORT, ("Master port, default to " + std::to_string(MASTER_PORT) + ".").c_str());
DEFINE_string(addr, "127.0.0.1", "Server (this program) address, default to localhost.");
DEFINE_int32(port, SERVER_PORT, ("Port # to listen on, default to " + std::to_string(SERVER_PORT) + ".").c_str());
DEFINE_bool(thrifty, false, "Use only as many messages as strictly required for inter-replica communication.");
DEFINE_bool(exec, false, "Execute commands.");
DEFINE_bool(dreply, false, "Reply to client only after command has been executed");
DEFINE_bool(beacon, false, "Send beacons to other replicas to compare their relative speeds.");
DEFINE_bool(durable, false, "Log to a stable store (i.e., a file in the current dir).");
DEFINE_validator(mport, &ValidatePort);
DEFINE_validator(port, &ValidatePort);


int main (int argc, char * argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    fprintf(stdout, "Server starting on port %d\n", FLAGS_port);

    Server s;
    registerWithMaster(FLAGS_maddr, FLAGS_mport, s);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    s.r = new Replica(s.Id, s.addrList, s.portList, FLAGS_thrifty, FLAGS_exec, FLAGS_dreply, FLAGS_beacon, FLAGS_durable);
    s.r->ListeningPort = FLAGS_port;

    if (!s.r->init()) {
        fprintf(stderr, "Cannot init replica! Exit...\n");
        exit(1);
    };

    int sock = listenOn(FLAGS_port + 1000); // master connect to this port
    if (sock < 0) {
        fprintf(stderr, "Cannot listen on %s:%d, Exit!\n", FLAGS_addr.c_str(), FLAGS_port + 1000);
        exit(1);
    }
    int on = sizeof(unsigned int);
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, on);  // make socket keeps alive
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&on, on);  // forbids NAGLE algorithm

    std::thread msp(handleMasterMessages, sock);
    s.r->run();
    msp.detach();
    fprintf(stdout, "Server Shutdown .............................................. DONE!\n");
    return 0;
}

void registerWithMaster(const std::string & addr, int port, Server & s) {
    RegisterArgs arg(FLAGS_addr, FLAGS_port);
    RegisterReply argr;
    bool done = false;
    while (!done) {
        int sock = dialTo(addr, port);
        if (sock < 0) {
            fprintf(stderr, "Cannot dial to master, retry after 3s...\n");
        } else {
            arg.Marshal(sock);
            uint8_t msgType;
            if (readUntil(sock, (char *) & msgType, 1) < 0 ) {
                fprintf(stderr, "Error happens when receiving RegisterReply from master...\n");
                continue;
            }
            if ((TYPE)msgType != REGISTER_REPLY) {
                fprintf(stderr, "Receive invalid msgType: %d...\n", msgType);
                continue;
            }
            if (!argr.Unmarshal(sock))
                fprintf(stderr, "Registration Reply cannot unmarshal right\n");
            else if (!argr.Ready)
                fprintf(stderr, "Registration Reply shows that replicas are not ready.\n");
            else {
                done = true;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    s.Id = argr.ReplicaId;
    s.addrList = argr.AddrList;
    s.portList = argr.PortList;
    fprintf(stdout, "Register on master ........................................... DONE!\n");
    return;
}

void handleMasterMessages(int sock) {
    int master;
    do {
        master = acceptAt(sock);
        if (master < 0) {
            fprintf(stderr, "Accept from master with wrong socket %d, retrying...\n", master);
        }
    } while (master < 0);
    fprintf(stdout, "Keep contact with master ..................................... DONE!\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    uint8_t msgType;
    GENERAL pr(PING_REPLY);
    BeTheLeaderReply br(true);
    for (;;) {
        readUntil(master, (char *)&msgType, 1);
        switch ((TYPE) msgType) {
            case PING:
                pr.Marshal(master);
                break;
            case BE_LEADER:
                br.Marshal(master);
                break;
            default:
                fprintf(stdout, "Unknown message type received from master.\n");
                break;
        }
    }
}