//
// Created by jingle on 17-4-12.
//
#include "../include/replica.h"
#include "../../utils/include/go_thread.h"
#include "../../utils/include/utils.h"
#include "../../execution/include/exec.h"

bool Replica::verify() {
    if(group_size == 0) {
        group_size = GROUP_SZIE;
    }
    if(group_size % 2 == 0) {
        error_exit(0, log_fp, "Group size must be odd !");
        return false;
    }

    if(checkpoint_cycle == 0) {
        checkpoint_cycle = CHECKPOINT_CYCLE;
    }
    if (PeerAddrList.size() == 0) {
        for (int i = 0; i < group_size; i++) {
            std::string addr = "localhost:" + to_string(PORT + i);
            PeerAddrList.push_back(addr);
        }
    }
    return true;
}

bool Replica::init() {

    if(!verify()){
        printf("init failed, more detail to see the log!\n");
        return false;
    }

    if(path.empty()){
        path = "/tmp/test" + to_string(Id);
    }
    InstanceMatrix = (tk_instance ***) malloc(group_size * sizeof(tk_instance **));
    crtInstance.resize(group_size, 0);
    executeUpTo.resize(group_size, 0);
    for(unsigned int i = 0; i < group_size; i++){
        InstanceMatrix[i] = (tk_instance **)malloc(2 * 1024 * 1024 * sizeof(tk_instance *));
        memset(InstanceMatrix[i], 0, 2 * 1024 * 1024 * sizeof(tk_instance *));
    }
    if(Restore){
        //TODO: recovery from log file;
    }

    return true;
}

int conflict, weird, slow, happy;
int cpcounter = 0;

bool Replica::run() {

    init();

    connectToPeers(this);
    info(stdout, "Waiting for client connections\n");
    go(waitForClientConnections, this);

    if (Exec) {
        go(execute_thread_t, this);
    }

    if (Id == 0)
        updatePreferredPeerOrder(this);

    go(slowClock, this);
    if (MAX_BATCH > 100)
        go(fastClock, this);

    if (Beacon)
        go(stopAdapting, this);

    int i;
    MsgQueue_t * pro_mq_s = pro_mq;
    while (!Shutdown) {
        if (pro_mq_s != NULL && hasNextMsg(pro_mq_s)) {
            void *msgp = getNextMsg(pro_mq_s);
            handlePropose(this, (Propose *) msgp);
            pro_mq_s = NULL;
        }
        if (pro_mq_s != NULL && !hasNextMsg(mq))
            continue;    // use a loop instead of select between two msgQueues
        void * msgp = getNextMsg(mq);
        uint32_t msgType = *(uint32_t *) msgp;
        switch (msgType) {
            case FAST_CLOCK:
                pro_mq_s = pro_mq;
                break;
            case PREPARE:
                // TODO: LOG
                handlePrepare(this, (Prepare *) msgp);
                break;
            case PREACCEPT:
                // TODO: LOG
                handlePreAccept(this, (PreAccept *) msgp);
                break;
            case ACCEPT:
                // ... So does the following cases
                handleAccept(this, (Accept *) msgp);
                break;
            case COMMIT:
                handleCommit(this, (Commit *) msgp);
                break;
            case COMMIT_SHORT:
                handleCommitShort(this, (CommitShort *) msgp);
                break;
            case PREPARE_REPLY:
                handlePrepareReply(this, (PrepareReply *) msgp);
                break;
            case PREACCEPT_REPLY:
                handlePreAcceptReply(this, (PreAcceptReply *) msgp);
                break;
            case PREACCEPT_OK:
                handlePreAcceptOK(this, (PreAcceptOk *) msgp);
                break;
            case ACCEPT_REPLY:
                handleAcceptReply(this, (AcceptReply *) msgp);
                break;
            case TRY_PREACCEPT:
                handleTryPreAccept(this, (TryPreAccept *) msgp);
                break;
            case TRY_PREACCEPT_REPLY:
                handleTryPreAcceptReply(this, (TryPreAcceptReply *) msgp);
                break;
            case BEACON:
                replyBeacon(this, (Beacon_msg *) msgp);
                break;
            case SLOW_CLOCK:
                if (Beacon) {
                    for (i = 0; i < group_size; i++) {
                        if (i == Id)
                            continue;
                        sendBeacon(this, i);
                    }
                }
                break;
            case CLIENT_CONNECT:
                weird = conflict = slow = happy = 0;
                break;
            case RECOVER_INSTANCE:
                startRecoveryForInstance(this, (InstanceId *) msgp);
                break;
        }
    }

    // TODO: 1. event loop; -- To process the message from servers each other, get msgs from a channel;

    //TODO:  2. excution Loop; -- periodically to apply cmds. build the KV;

    //TODO:  3. propose Loop;  -- client raise a proposal, when server receives a cmd, process it, and put it in a Propose channel;

    //TODO:  4. Timeout Loop;  -- check timeout,

    //TODO:  network start;    --  connect peer and peer, and process the message.

    //coroutine method;
    return true;
}


// ********************************* Threads ********************************************

// TODO
void * waitForClientConnections(void * arg) {
    replica_server_param_t * r = (replica_server_param_t *) arg;
    ClientConnect_t * notice = (ClientConnect_t *)malloc(sizeof(ClientConnect_t));
    notice->type = CLIENT_CONNECT;
    while (!(r->flag & SHUTDOWN_MASK)) {
        // accept requests from clients
        // go(clientListener);
        putIntoMsgQueue(r->mq, &notice);
    }
    free(notice);
}

void * slowClock(void * arg) {
    replica_server_param_t * r = (replica_server_param_t *) arg;
    Clock_t * timeout = (Clock_t *)malloc(sizeof(Clock_t));
    timeout->type = SLOW_CLOCK;
    while (!(r->flag & SHUTDOWN_MASK)) {
        nano_sleep(150 * 1000000); // 150 ms
        putIntoMsgQueue(r->mq, &timeout);
    }
    free(timeout);
}

void * fastClock(void * arg) {
    Replica * r = (Replica *) arg;
    Clock * timeout = (Clock *)malloc(sizeof(Clock));
    timeout->Type = FAST_CLOCK;
    while (!r->Shutdown) {
        nano_sleep(5 * 1000000); // 5 ms
        putIntoMsgQueue(r->mq, &timeout);
    }
    free(timeout);
}

void * stopAdapting(void * arg) {
    nano_sleep(1000 * 1000 * 1000 * ADAPT_TIME_SEC);
    Replica * r = (Replica *) arg;
    r->Beacon = false;
    nano_sleep(1000 * 1000 * 1000);

    int i, j, min;
    uint32_t temp;
    for (i = 0; i < r->group_size - 1; i++) {
        min = i;
        for (j = i+1; j < r->group_size - 1; j++) {
            if (r->Ewma[r->PreferredPeerOrder[j]] < r->Ewma[r->PreferredPeerOrder[min]])
                min = j;
        }
        temp = r->PreferredPeerOrder[i];
        r->PreferredPeerOrder[i] = r->PreferredPeerOrder[min];
        r->PreferredPeerOrder[min] = temp;
    }
}

// ********************************* Functions *******************************************

void updatePreferredPeerOrder(Replica * r) {
    uint32_t * quorum = (uint32_t *)malloc(sizeof(uint32_t) * (1 + (r->group_size >> 1)));
    uint32_t * aux = (uint32_t *) malloc(sizeof(uint32_t) * r->group_size);
    uint32_t i, j = 0, k = 0, p;
    for (i = 0; i <= (r->group_size>>1); i++) {
        quorum[i] = i;
        if (i == r->Id)
            continue;
        aux[j++] = i;
    }
    for (i = 0; i < r->group_size; i++) {
        p = r->PreferredPeerOrder[i];
        uint8_t found = 0;
        for (j = 0; j < i; j++) {
            if (aux[j] == p) {
                found = 1;
                break;
            }
        }
        if (!found) {
            aux[k++] = p;
        }
    }
    for (i = 0; i < r->group_size; i++) {
        r->PreferredPeerOrder[i] = aux[i];
    }
    free(quorum);
    free(aux);
}

// ********************************** Handlers ******************************************

/****************************************************************************************
                                      PHASE 1
****************************************************************************************/
void handlePropose(Replica * r, Propose * msgp) {
    long batchSize = availableMsgCount(r->pro_mq) + 1;
    if (batchSize > MAX_BATCH) {
        batchSize = MAX_BATCH;
    }
    uint64_t instNo = r->crtInstance[r->Id];
    r->crtInstance[r->Id]++;
    // LOG: starting instance [instNo]
    // LOG: batching [batchSize]

    tk_command * cmds = new tk_command[batchSize];
    Propose * proposals = (Propose *) malloc (sizeof(Propose) * batchSize);
    cmds[0] = msgp->Command;
    proposals[0] = *msgp;
    free(msgp);
    // the original messages can be freed since they are already copied
    // allocate a new block of space for cmds/proposals for using cache
    // since the original messages may be separated in many distinct blocks
    int i;
    for (i = 1; i < batchSize; i++) {
        msgp = *(Propose **)getNextMsg(r->pro_mq);
        cmds[i] = msgp->Command;
        proposals[i] = *msgp;
        free(msgp);
    }
    startPhase1(r, instNo, 0, proposals, cmds, batchSize);
}

void startPhase1(Replica * r, uint64_t instance,
                 uint32_t ballot, Propose * proposals,
                 tk_command * cmds, long batchSize) {
    // init command attributes
    int q;
    unsigned int seq = 0, * deps = (unsigned int *) malloc (sizeof(unsigned int) * r->group_size);
    for (q = 0; q < r->group_size; q++) {
        deps[q] = 0;
    }

    updateAttributes(r, batchSize, cmds, &seq, deps, r->Id, instance);

    tk_instance newInstance = {cmds, batchSize, deps, seq, 0, 0, NULL};
    lb_t * lb = (lb_t *)malloc(sizeof(lb_t));
    uint32_t * committedDeps = (uint32_t *) malloc (sizeof(uint32_t) * r->group_size);
    memset(lb, 0, sizeof(lb_t));
    lb->clientProposals = proposals;
    lb->allEqual = 1;
    lb->originalDeps = deps;
    lb->committedDeps = committedDeps;
    newInstance.lb = lb;
    r->InstanceMatrix[r->Id][instance] = newInstance;

    updateConflicts(r, batchSize, cmds, r->Id, instance, seq);

    if (seq >= r->maxSeq) {
        r->maxSeq = seq + 1;
    }

    /* TODO
     * record into stable storage
     * recordInstanceMetaData(r->InstanceMatrix[r->replicaId][instance]);
     * recordCommands(cmds);
     * sync(); // sync with stable storage
     */

    bcastPreAccept(r, r->replicaId, instance, ballot, cmds, seq, deps);
    cpcounter += batchSize;

    if (r->replicaId == 0 && DO_CHECKPOINTING && cpcounter >= CHECKPOINT_PERIOD) {
        // by default this block will not execute
        /*
         * cpcounter = 0;
         * r->MaxInstanceNum[r->replicaId]++;
         * instance++;
         * r->maxSeq++;
         * ....  ignored here
         */
    }
}

// TODO
void bcastPreAccept() {
    // Broadcast PreAccept
}

// ********************************* Helper Functions **********************************
void updateAttributes(Replica * r, long len, tk_command * cmds,
                      unsigned int * seq, unsigned int * deps, int replica, uint64_t instance) {
    uint8_t changed = 0;
    int i, q;
    for (q = 0; q < r->group_size; q++) {
        if (r->Id != replica && q == replica)
            continue;
        for (i = 0; i < len; i++) {
            auto p = (r->conflicts[q]).find(std::string(cmds[i].key));
            if (p != r->conflicts[q].end()) {
                if (p->second > deps[q]) {
                    deps[q] = p->second;
                    if (*seq <= r->InstanceMatrix[q][p->second]->seq) {
                        *seq = r->InstanceMatrix[q][p->second]->seq + 1;
                    }
                    changed = 1;
                    break;
                }
            }
        }
    }
    for (i = 0; i < len; i++) {
        auto s = r->maxSeqPerKey.find(std::string(cmds[i].key));
        if (s != r->maxSeqPerKey.end()) {
            changed = 1;
            *seq = s->second + 1;
        }
    }
}

void updateConflicts(Replica * r, long len, tk_command * cmds,
                     uint8_t replica, uint64_t instance, unsigned int seq) {
    int i;
    for (i = 0; i < len; i++) {
        auto d = (r->conflicts[replica]).find(std::string(cmds[i].key));
        if (d != r->conflicts[replica].end()) {
            if (d->second < instance)
                r->conflicts[replica][std::string(cmds[i].key)] = instance;
        } else {
            r->conflicts[replica][std::string(cmds[i].key)] = instance;
        }
        auto s = r->maxSeqPerKey.find(std::string(cmds[i].key));
        if (s != r->maxSeqPerKey.end() && s->second < seq) {
            r->maxSeqPerKey[std::string(cmds[i].key)] = seq;
        } else {
            r->maxSeqPerKey[std::string(cmds[i].key)] = seq;
        }
    }
}