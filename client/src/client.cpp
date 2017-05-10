//
// Created by forec on 17-5-8.
//

#include "../include/client.h"
#include <gflags/gflags.h>
#include <bits/unordered_map.h>

DEFINE_string(maddr, "127.0.0.1", "Master address, default to localhost.");
DEFINE_int32(mport, MASTER_PORT, ("Master port, default to " + std::to_string(MASTER_PORT) + ".").c_str());
DEFINE_int32(requests, 5000, "Total number of requests, default to 5000.");
DEFINE_int32(w, 100, "Percentagee of updates(writes), default to 100.");
DEFINE_int32(r, 1, "Split the total number of requests into this many rounds, and do rounds "
    "sequentially, default to");
DEFINE_int32(eps, 0, "Send eps more messages per round than the client will wait for (to discount "
    "stragglers), default to 0.");
DEFINE_int32(conflicts, 0, "Percentage of conflicts, default to 0%.");
DEFINE_bool(check, false, "Check that every expected reply was received exactly once.");
DEFINE_bool(thrifty, false, "Use only as many messages as strictly required for inter-replica communication.");
DEFINE_bool(exec, false, "Execute commands.");
DEFINE_bool(dreply, false, "Reply to client only after command has been executed");
DEFINE_bool(beacon, false, "Send beacons to other replicas to compare their relative speeds.");
DEFINE_bool(durable, false, "Log to a stable store (i.e., a file in the current dir).");
DEFINE_validator(mport, &ValidatePort);
DEFINE_validator(conflicts, &ValidatePercent);
DEFINE_validator(w, &ValidatePercent);

int N;
std::vector<int> successful;
std::vector<int> rarray;
std::vector<bool> rsp;

int main(int argc, char * argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    int master = dialTo(FLAGS_maddr, FLAGS_mport);
    if (master < 0) {
        fprintf(stderr, "Error connecting to master!\n");
        exit(1);
    }

    uint8_t msgType;
    GetReplicaListArgs rl;
    GetReplicaListReply rlp;
    rl.Marshal(master);
    if (readUntil(master, (char *) &msgType, 1) != 0 || (TYPE)msgType != GET_REPLICA_LIST_REPLY ||
        !rlp.Unmarshal(master)) {
        fprintf(stderr, "Error when getting replica lists from master!\n");
        exit(1);
    }
    if (!rlp.Ready) {
        fprintf(stderr, "Master's reply shows that replicas are not ready!\n");
        exit(0);
    }

    N = (int)rlp.ReplicaAddrList.size();
    std::vector<RDMA_CONNECTION> servers(N, -1);
    std::vector<std::string> karray((unsigned long)FLAGS_requests / FLAGS_r + FLAGS_eps, "");
    std::vector<int> rarray((unsigned long)FLAGS_requests / FLAGS_r + FLAGS_eps, -1);
    std::vector<bool> put((unsigned long)FLAGS_requests / FLAGS_r + FLAGS_eps, false);
    std::vector<int> perReplicaCount(N, 0);
    std::unordered_map<std::string, int> test;

    srand((unsigned int)time(nullptr));

    for (int i = 0; i < rarray.size(); i++) {
        int r = rand() % N;
        rarray[i] = r;
        if (i < FLAGS_requests / FLAGS_r) {
            perReplicaCount[r] ++;
        }
        if (FLAGS_conflicts > 0) {
            r = rand() % 100;
            if (r < FLAGS_conflicts) {
                karray[i] = CONFLICT_KEY;
            } else {
                karray[i] = TEST_KEY + std::to_string(i);
            }
            r = rand() % 100;
            put[i] = false;
            if (r < FLAGS_w)
                put[i] = true;
        } else {
            karray[i] = TEST_KEY + std::to_string(i);
            test[karray[i]] ++;
        }
    }

    if (FLAGS_conflicts > 0) {
        fprintf(stdout, "Conflict = %d%% distribution\n", FLAGS_conflicts);
    } else {
        fprintf(stdout, "Non-same distribution\n");
    }

    for (int i = 0; i < N; i++) {
        servers[i] = dialTo(rlp.ReplicaAddrList[i], (uint16_t )rlp.ReplicaPortList[i]);
        if (servers[i] < 0) {
            fprintf(stderr, "Error connecting to replica %d\n", i);
            continue;
        }
        int on = sizeof(unsigned int);
        setsockopt(servers[i], SOL_SOCKET, SO_KEEPALIVE, (void *)&on, on);  // make socket keeps alive
        setsockopt(servers[i], IPPROTO_TCP, TCP_NODELAY, (void *)&on, on);  // forbids NAGLE algorithm
    }

    std::vector<int> successfull(N, 0);

    int32_t id = 0;
    MsgQueue * mq = new MsgQueue(N, 1);
    tk_command tkc;
    Propose args(id, 0, tkc, -1);

    clock_t before_total = clock();
    char buf[MAX_VALUE_SIZE + 1] = {0};

    for (int j = 0; j < FLAGS_r; j++) {
        int n = FLAGS_requests / FLAGS_r;
        if (FLAGS_check) {
            rsp.resize(n, false);
        }

        std::vector<std::thread *> wrts;
        wrts.clear();
        for (int i = 0; i < N; i++) {
            wrts.push_back(new std::thread(waitReplies, std::ref(servers), i, perReplicaCount[i], mq));
        }

        clock_t before = clock();

        for (int i = 0; i < n + FLAGS_eps; i++) {
            fprintf(stdout, "Sending proposal %d\n", id);
            args.CommandId = id;
            if (put[i])
                args.Command.opcode = PUT;
            else
                args.Command.opcode = GET;
            args.Command.key = karray[i];
            std::string _v = randStr();
            strcpy(buf, _v.c_str());
            args.Command.val = buf;
            args.Command.valSize = (int32_t)_v.size() + 1;
            fprintf(stdout, "Ready to send proposal %d to replica %d\n", id, rarray[i]);
            args.Marshal(servers[rarray[i]]);
            id++;
        }

        bool succ = true;
        for (int i = 0; i < N; i++) {
            succ |= (bool)mq->get();
        }

        for (std::thread * wrt: wrts) {
            wrt->detach();
            delete wrt;
        }

        clock_t after = clock();
        fprintf(stdout, "Round took %ld\n", ((unsigned long)after - before) / CLOCKS_PER_SEC);

        if (FLAGS_check) {
            for (int j = 0; j < n; j++) {
                if (!rsp[j]) {
                    fprintf(stderr, "Didn't receive from replica %d\n", j);
                }
            }
        }

        if (!succ) {
            N--;
        }
    }

    clock_t after_total = clock();
    fprintf(stdout, "Test took %ld\n", (after_total - before_total) / CLOCKS_PER_SEC);

    int sum = 0;
    for (int successCount : successfull) {
        sum += successCount;
    }
    fprintf(stdout, "Successful: %d\n", sum);

    for (int sock: servers) {
        if (sock >= 0)
            destroyConnection(sock);
    }
    destroyConnection(master);
    return 0;
}

void waitReplies(std::vector<RDMA_CONNECTION> & servers, int leader, int n, MsgQueue * mq) {
    bool e = false;
    ProposeReplyTS reply;
    for (int i = 0; i < n; i++) {
        uint8_t msgType;
        readUntil(servers[i], (char *) &msgType, 1);
        if ((TYPE)msgType != PROPOSE_REPLY_TS || !reply.Unmarshal(servers[i])) {
            fprintf(stderr, "Error when getting propose reply from replica %d\n", i);
            e = true;
            continue;
        }
        if (FLAGS_check) {
            if (rsp[reply.CommandId]) {
                fprintf(stderr, "Duplicate reply for command %d\n", reply.CommandId);
            }
            rsp[reply.CommandId] = true;
        }
        if (reply.OK) {
            successful[leader] ++;
        }
    }
    uint8_t true_u = 0;
    if (!e)
        true_u = 0xff;
    mq->put(&true_u);
}

std::string randStr(int least, int most) {
    int size = least + (rand() % (most - least));
    std::string res((unsigned long)size, 'A');
    for (int i = 0; i < size; i++) {
        res[i] += rand() % 26;
    }
    return res;
}