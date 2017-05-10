#include "../include/replica.h"
#include "../../execution/include/exec.h"
#include <time.h>

int conflict, weird, slow, happy;
int cpcounter = 0;

std::vector<tk_command> emptyVec_cmd;
std::vector<Propose> emptyVec_pro;
std::vector<int32_t> emptyVec_int;
std::vector<bool> emptyVec_bool;
std::unordered_map<uint64_t, uint64_t> deferMap;

PreAccept pa;
TryPreAccept tpa;
Prepare pr;
Accept ea;
Commit cm;
CommitShort cms;

Replica::Replica() {
}

Replica::Replica(int32_t _rId, std::vector<std::string> & _addrList, std::vector<int> & _portList,
                 bool _thrifty, bool _exec, bool _dreply, bool _beacon, bool _durable) :
    Id(_rId), PeerAddrList(_addrList), PeerPortList(_portList), Thrifty(_thrifty),
    Exec(_exec), Dreply(_dreply), Beacon(_beacon), Durable(_durable) {
}

bool Replica::verify() {
    if(group_size == 0) {
        group_size = GROUP_SZIE;
    }
    if(group_size % 2 == 0) {
        fprintf(stderr, "Group size must be odd!\n");
        return false;
    }

    if (ListeningPort < 0) {
        ListeningPort = SERVER_PORT;
    }
    if(checkpoint_cycle == 0) {
        checkpoint_cycle = CHECKPOINT_CYCLE;
    }
    if (PeerAddrList.size() == 0) {
        for (int i = 0; i < group_size; i++) {
            PeerAddrList.push_back("localhost");
            PeerPortList.push_back(SERVER_PORT + i);
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
        path = "/tmp/test" + std::to_string(Id);
    }
    Shutdown = false;
    Restore = false;
    InstanceMatrix = (tk_instance ***) malloc(group_size * sizeof(tk_instance **));
    crtInstance.resize((unsigned long)group_size, 0);
    executeUpTo.resize((unsigned long)group_size, 0);
    mq = new MsgQueue(1024 * 1024 ,8);
    pro_mq = new MsgQueue(1024 * 1024 ,8);
    Peers.resize((unsigned long)group_size, -1);
    Alive.resize((unsigned long)group_size, false);
    PreferredPeerOrder.resize((unsigned long)group_size, 0);
    Ewma.resize((unsigned long)group_size, 0.0);
    for(unsigned int i = 0; i < group_size; i++){
        InstanceMatrix[i] = (tk_instance **)malloc(1024 * 1024 * sizeof(tk_instance *));
        memset(InstanceMatrix[i], 0, 1024 * 1024 * sizeof(tk_instance *));
        conflicts.push_back(std::unordered_map<std::string, int32_t>());
        PreferredPeerOrder[i] = int32_t(Id + 1 + i) % group_size;
    }
    if(Restore){
        //TODO: recovery from log file; (original lost)
    }

    return true;
}

bool Replica::run() {

    init();

    std::vector<std::thread *> threads;

    connectToPeers(threads);

    threads.push_back(new std::thread(waitForClientConnections, this));

    fprintf(stdout, "Wait for client connections: DONE!\n");

    if (Exec) {
        threads.push_back(new std::thread(execute_thread, this));
        fprintf(stdout, "Start execution loop: DONE!\n");
    }

    if (Id == 0) {
        std::vector<int32_t> quorum((unsigned long)(group_size / 2 + 1), 0);
        for (int i = 0; i <= group_size/2; i++)
            quorum[i] = i;
        updatePreferredPeerOrder(quorum);
        fprintf(stdout, "Update preferred order: DONE!\n");
    }

    threads.push_back(new std::thread(fastClock, this));
    fprintf(stdout, "Start fast clock: DONE!\n");

    if (Beacon) {
        threads.push_back(new std::thread(slowClock, this));
        fprintf(stdout, "Start slow clock: DONE!\n");
        threads.push_back(new std::thread(stopAdapting, this));
        fprintf(stdout, "Stop adapting: DONE!\n");
    }

    int i;
    MsgQueue * pro_mq_s = pro_mq;

    fprintf(stdout, "Start message dispatcher: DONE!\n");

    while (!Shutdown) {
        if (pro_mq_s != NULL && pro_mq_s->hasNext()) {
            fprintf(stdout, "try to get msg from pro_mq\n");
            void *msgp = pro_mq_s->get();
            fprintf(stdout, "get msgp!!!!\n");
            handlePropose((Propose *) msgp);
            fprintf(stdout, "handle over msgp!!!!\n");
            pro_mq_s = NULL;
        }
        fprintf(stdout, "mq->hasNext() : %d\n", mq->hasNext());
        if (pro_mq_s != NULL && !mq->hasNext())
            continue;    // use a loop instead of select between two msgQueues

        void * msgp = mq->get();
        TYPE msgType = *(TYPE *) msgp;
        fprintf(stdout, "msgType =%d\n", msgType);
        switch (msgType) {
            case FAST_CLOCK:
                fprintf(stdout, "receive fast clock!\n");
                pro_mq_s = pro_mq;
                break;
            case PREPARE:
                // TODO: LOG
                handlePrepare((Prepare *) msgp);
                break;
            case PREACCEPT:
                // TODO: LOG
                handlePreAccept((PreAccept *) msgp);
                break;
            case ACCEPT:
                // ... So does the following cases
                handleAccept((Accept *) msgp);
                break;
            case COMMIT:
                handleCommit((Commit *) msgp);
                break;
            case COMMIT_SHORT:
                handleCommitShort((CommitShort *) msgp);
                break;
            case PREPARE_REPLY:
                handlePrepareReply((PrepareReply *) msgp);
                break;
            case PREACCEPT_REPLY:
                handlePreAcceptReply((PreAcceptReply *) msgp);
                break;
            case PREACCEPT_OK:
                handlePreAcceptOK((PreAcceptOk *) msgp);
                break;
            case ACCEPT_REPLY:
                handleAcceptReply((AcceptReply *) msgp);
                break;
            case TRY_PREACCEPT:
                handleTryPreAccept((TryPreAccept *) msgp);
                break;
            case TRY_PREACCEPT_REPLY:
                handleTryPreAcceptReply((TryPreAcceptReply *) msgp);
                break;
            case BEACON:
                replyBeacon((Beacon_msg *) msgp);
                break;
            case SLOW_CLOCK:
                if (Beacon) {
                    for (i = 0; i < group_size; i++) {
                        if (i == Id)
                            continue;
                        sendBeacon(i);
                    }
                }
                break;
            case CLIENT_CONNECT:
                weird = conflict = slow = happy = 0;
                break;
            case RECOVER_INSTANCE:
                startRecoveryForInstance(((InstanceId *) msgp)->replica, ((InstanceId *) msgp)->instance);
                break;
            default:
                break;
        }
    }

    //DONE:  1. event loop;    -- To process the message from servers each other, get msgs from a channel;
    //DONE:  2. excution Loop; -- periodically to apply cmds. build the KV;
    //DONE:  3. propose Loop;  -- client raise a proposal, when server receives a cmd, process it, and put it in a Propose channel;
    //DONE:  4. Timeout Loop;  -- check timeout,
    //DONE:  5. network start; -- connect peer and peer, and process the message.
    //TODO:  6. network communication


    for (i = 0; i < threads.size(); i++) {
        if (threads[i]->joinable())
            threads[i]->detach();
        delete threads[i];
    }
    return true;
}


/***************************************************************************************
 *                                     Threads                                         *
 **************************************************************************************/

void waitForClientConnections(Replica * r) {
    ClientConnect * notice = new ClientConnect();
    std::vector<std::thread *> clientListeners;
    while (!r->Shutdown) {
        // TODO - DONE
        // accept requests from clients
        // RDMA_CONNECTION conn = r->Listener->accept();
        // _ClientParam * cpr = new _ClientParam(r, conn);
        // go(clientListener, cpr);

        // TEST
        int sock = acceptAt(r->Listener);
        fprintf(stdout, "Connect from client on sock %d\n", sock);
        clientListeners.push_back(new std::thread(clientListener, r, sock));
        r->mq->put(&notice);
    }
    for (std::thread * cl: clientListeners) {
        cl->detach();
        delete cl;
    }
    delete notice;
}

void clientListener(Replica * r, RDMA_CONNECTION conn) {
    uint8_t msgType;
    Read read;
    ProposeAndRead pandr;
    Propose * prop;

    while (!r->Shutdown) {
        // TODO - DONE
        // msgType = reader->ReadByte();
        // if ERROR: return;
        // TEST
        readUntil(conn, (char *)&msgType, 1);

        switch ((TYPE)(msgType)) {
            case PROPOSE: // All commands are proposed from PROPOSE.
                prop = new Propose();
                if ( !prop->Unmarshal(conn) )
                    return;
                r->pro_mq->put(&prop);
                break;
            case READ:  // useless
//          if (ERROR == read.Unmarshal(conn))
//              return;
                break;
            case PROPOSE_AND_READ:  // useless
//          if (ERROR == pandr.Unmarshal(conn))
//              return;
                break;
            default:
                // LOG("Unknown msg type");
                break;
        }
    }
}

void waitForPeerConnections(Replica * r, bool * done) {
    // TODO - DONE
    // r->Listener = RDMA_Listen("tcp", r->peerAddrList[Id]);
    // TEST
    do {
        r->Listener = listenOn(r->ListeningPort);
        if (r->Listener < 0) {
            fprintf(stderr, "Cannot listen on 0.0.0.0:%d\n", r->ListeningPort);
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    } while (r->Listener < 0);

    for (int i = r->Id + 1; i < r->group_size; i++) {
        // TODO - DONE
        // RDMA_CONNECTION conn = r->Listener.Accept();
        // RDMA_ReadFull(conn, b);
        // TEST
        RDMA_CONNECTION conn = acceptAt(r->Listener);
        if (conn < 0) {
            fprintf(stderr, "Error when accept connect request from other replicas\n");
            continue;
        }
        int32_t id;
        readUntil(conn, (char *) &id, 4);
        fprintf(stdout, "Connect with replica %d: DONE!\n", id);
        r->Peers[id] = conn;
        // r->PeerReaders[id] = NewReader(conn)
        // r->PeerWriters[id] = NewWriter(conn)
        r->Alive[id] = true;
    }
    *done = true;
}

void replicaListener(Replica * r, int32_t rid, RDMA_CONNECTION conn) {
    uint8_t msgType;
    Beacon_msg gbeacon;
    Beacon_msg_reply gbeaconReply;
    Beacon_msg * beacon;

    while (!r->Shutdown) {
        // TODO - DONE
        // msgType = reader->ReadByte();
        // if ERROR: return;
        // TEST
        readUntil(conn, (char *)&msgType, 1);
        fprintf(stdout, "receive msgType :%d\n", msgType);
        fflush(stdout);

        switch ((TYPE)(msgType)) {
            case BEACON:
                if ( !gbeacon.Unmarshal(conn) )
                    return;
                beacon = new Beacon_msg(rid, gbeacon.timestamp);
                r->mq->put(&beacon);
                break;
            case BEACON_REPLY:
                if ( !gbeaconReply.Unmarshal(conn) )
                    return;
                r->Ewma[rid] = 0.99*r->Ewma[rid] + 0.01*(double)(CPUTicks() - gbeaconReply.timestamp);
                break;
            default:
                // TODO - DONE
                // find matched msg Type
                // if find: unmarshal new msg
                // else: LOG("Unknown msg type");
                // putIntoMsgQueue(r->mq, msg);
                // TEST
                msgDispatcher(r, rid, conn, (TYPE)msgType);
                break;
        }
    }
}

void slowClock(Replica * r) {
    Clock * timeout = new Clock(SLOW_CLOCK);
    while (!r->Shutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(150)); // 150 ms
        r->mq->put(&timeout);
    }
    delete timeout;
}

void fastClock(Replica * r) {
    Clock * timeout = new Clock(FAST_CLOCK);
    while (!r->Shutdown) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 5 ms
        r->mq->put(&timeout);
    }
    delete timeout;
}

void stopAdapting(Replica * r) {
    std::this_thread::sleep_for(std::chrono::seconds(ADAPT_TIME_SEC));
    r->Beacon = false;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < r->group_size - 1; i++) {
        int min = i;
        for (int j = i+1; j < r->group_size - 1; j++) {
            if (r->Ewma[r->PreferredPeerOrder[j]] < r->Ewma[r->PreferredPeerOrder[min]])
                min = j;
        }
        int32_t temp = r->PreferredPeerOrder[i];
        r->PreferredPeerOrder[i] = r->PreferredPeerOrder[min];
        r->PreferredPeerOrder[min] = temp;
    }
}


/****************************************************************************************
 *                                     PHASE 1                                          *
****************************************************************************************/
void Replica::handlePropose(Propose * propose) {
    long batchSize = pro_mq->count() + 1;

    if (batchSize <= 0)
        return;
    if (batchSize > MAX_BATCH) {
        batchSize = MAX_BATCH;
    }

    int32_t instNo = crtInstance[Id];
    crtInstance[Id]++;
    // TODO
    // LOG: starting instance [instNo]
    // LOG: batching [batchSize]

    std::vector<tk_command> cmds((unsigned long)batchSize, tk_command());
    std::vector<Propose> proposals((unsigned long)batchSize, Propose());
    cmds[0] = propose->Command;
    proposals[0] = *propose;
    delete propose;
    // the original messages can be freed since they are already copied
    // allocate a new block of space for cmds/proposals for using cache
    // since the original messages may be separated in many distinct blocks
    for (int i = 1; i < batchSize; i++) {
        propose = *(Propose **)pro_mq->get();
        cmds[i] = propose->Command;
        proposals[i] = *propose;
        delete propose;
    }

    fprintf(stdout, "Ready to start phase1\n");
    startPhase1(Id, instNo, 0, proposals, cmds, batchSize);
}

void Replica::startPhase1(int32_t replica, int32_t instance, int32_t ballot,
                          std::vector<Propose> & proposals,
                          std::vector<tk_command> & cmds, long batchSize) {
    // init command attributes
    int32_t  seq = 0;
    std::array<int32_t, GROUP_SZIE> deps;
    for (int q = 0; q < group_size; q++) {
        deps[q] = -1;
    }

    updateAttributes(cmds, seq, deps, Id, instance);
    fprintf(stdout, "Finish update attributes!\n");

    std::vector<int32_t> _cmDeps(5, -1);
    lb_t * lb = new lb_t(proposals, 0, 0, true, 0, 0, 0, deps, _cmDeps, nullptr, false, false, emptyVec_bool, 0);
    tk_instance * newInstance = new tk_instance(cmds, ballot, PREACCEPTED, seq, deps, lb);
    InstanceMatrix[Id][instance] = newInstance;

    updateConflicts(cmds, Id, instance, seq);
    fprintf(stdout, "Finish update conflicts!\n");

    if (seq >= maxSeq) {
        maxSeq = seq + 1;
    }

    /* TODO
     * record into stable storage
     * recordInstanceMetaData(InstanceMatrix[Id][instance]);
     * recordCommands(cmds);
     * sync(); // sync with stable storage
     */

    bcastPreAccept(Id, instance, ballot, cmds, seq, deps);
    cpcounter += batchSize;

    if (Id == 0 && DO_CHECKPOINTING && cpcounter >= CHECKPOINT_PERIOD) {
        // by default this block will not execute
        /*
         * cpcounter = 0;
         * crtInstance[Id]++;
         * instance++;
         * maxSeq++;
         * ....  ignored here
         */
    }
}

void Replica::handlePreAccept(PreAccept * preAccept) {
    tk_instance * inst = InstanceMatrix[preAccept->LeaderId][preAccept->Instance];

    if (preAccept->Seq >= maxSeq)
        maxSeq = preAccept->Seq + 1;

    if (inst != nullptr && (inst->status == COMMITTED || inst->status == ACCEPTED)) {
        if (inst->cmds.empty()) {
            inst->cmds = preAccept->Command;
            updateConflicts(preAccept->Command, preAccept->Replica, preAccept->Instance, preAccept->Seq);
        }
        // TODO
        // recordCommands(preAccept->cmds, preAccept->cmds_count);
        // sync(r);
        return;
    }
    if (preAccept->Instance >= crtInstance[preAccept->Replica])
        crtInstance[preAccept->Replica] = preAccept->Instance + 1;

    // update attributes for command
    int32_t seq = preAccept->Seq;
    std::array<int32_t, GROUP_SZIE> deps = preAccept->Deps;
    bool changed = updateAttributes(preAccept->Command, seq, deps, preAccept->Replica, preAccept->Instance);
    bool uncommittedDeps = false;

    for (int q = 0; q < group_size; q++) {
        if (deps[q] > committedUpTo[q]) {
            uncommittedDeps = true;
            break;
        }
    }
    STATUS status = PREACCEPTED_EQ;
    if (changed) {
        status = PREACCEPTED;
    }

    if (inst != nullptr) {
        if (preAccept->Ballot < inst->ballot) {
            PreAcceptReply * preacply= new PreAcceptReply(preAccept->Replica, preAccept->Instance, false,
                                      inst->ballot, inst->seq, inst->deps, committedUpTo);
            replyPreAccept(preAccept->LeaderId, preacply);
            return;
        } else {
            inst->cmds = preAccept->Command;
            inst->seq = seq;
            inst->deps = deps;
            inst->ballot = preAccept->Ballot;
            inst->status = status;
        }
    } else {
        tk_instance * newInstance = new tk_instance(preAccept->Command, preAccept->Ballot,
                                                    status, seq, deps, nullptr);
        InstanceMatrix[preAccept->Replica][preAccept->Instance] = newInstance;
    }

    updateConflicts(preAccept->Command, preAccept->Replica, preAccept->Instance, preAccept->Seq);

    // TODO:: Record into stable storage
    // recordInstanceMetadata(InstanceSpace[preAccept->Replica][preAccept->Instance])
    // recordCommands(preAccept->Command)
    // sync()

    if (preAccept->Command.empty()) {
        latestCPReplica = preAccept->Replica;
        latestCPInstance = preAccept->Instance;
        // discard dependency hash tables
        clearHashTables();
    }

    if (changed || uncommittedDeps || preAccept->Replica != preAccept->LeaderId ||
        !isInitialBallot(preAccept->Ballot)) {
        PreAcceptReply * preacply= new PreAcceptReply(preAccept->Replica, preAccept->Instance, true,
                                                      preAccept->Ballot, seq, deps, committedUpTo);
        // TODO - DONE
        // replyPreAccept(preAccept->LeaderId, preacply);
        // TEST
        replyPreAccept(preAccept->LeaderId, preacply);
    } else {
        PreAcceptOk * pok = new PreAcceptOk(preAccept->Instance);
        // TODO - DONE
        // sendMsg(preAccept->LeaderId, pok);
        // TEST
        pok->Marshal(preAccept->LeaderId);
    }
    // TODO - DONE
    // LOG("I have replied to the PreAccept\n");
    // TEST
    fprintf(stdout, "I have replied to the PreAccept\n");
}

void Replica::handlePreAcceptReply(PreAcceptReply * pareply) {
    // TODO - DONE
    // LOG("Handling PreAccept Reply");
    // TEST
    fprintf(stdout, "Handling PreAccept Reply\n");
    tk_instance * inst = InstanceMatrix[pareply->Replica][pareply->Instance];

    if (inst == nullptr || inst->status == PREACCEPTED) {
        // we have moved on, this is a delayed reply
        return;
    }

    if (inst->ballot != pareply->Ballot)
        return;
    if (!pareply->Ok) {
        // TODO: there is probably another active leader (original lost)
        inst->lb->nacks++;
        if (pareply->Ballot > inst->lb->maxRecvBallot)
            inst->lb->maxRecvBallot = pareply->Ballot;
        if (inst->lb->nacks >= group_size / 2) {
            // TODO (original lost)
        }
        return;
    }

    inst->lb->preAcceptOKs++;
    bool equal = mergeAttributes(inst->seq, inst->deps, pareply->Seq, pareply->Deps);
    if ((group_size <= 3 && !Thrifty) || inst->lb->preAcceptOKs > 1) {
        inst->lb->allEqual = inst->lb->allEqual & equal;
        if (!equal) {
            conflict++;
        }
    }
    bool allCommitted = true;
    for (int q = 0; q < group_size; q++) {
        if (inst->lb->committedDeps[q] < pareply->CommittedDeps[q])
            inst->lb->committedDeps[q] = pareply->CommittedDeps[q];
        if (inst->lb->committedDeps[q] < committedUpTo[q])
            inst->lb->committedDeps[q] = committedUpTo[q];
        if (inst->lb->committedDeps[q] < inst->deps[q])
            allCommitted = false;
    }

    // can we commit on the fast path?
    if (inst->lb->preAcceptOKs >= group_size / 2 &&
        inst->lb->allEqual && allCommitted &&
        isInitialBallot(inst->ballot)) {
        happy++;
        // TODO - DONE
        // LOG("Fast path for instance %d.%d\n", pareply->Replica, pareply->Instance);
        // TEST
        fprintf(stdout, "Fast Path For Instance %d.%d\n", pareply->Replica, pareply->Instance);
        InstanceMatrix[pareply->Replica][pareply->Instance]->status = COMMITTED;
        updateCommitted(pareply->Replica);
        if (!inst->lb->clientProposals.empty() && !Dreply) {
            // give clients the all clear
            for (int i = 0; i < inst->lb->clientProposals.size(); i++) {
                ProposeReplyTS * prts = new ProposeReplyTS(true, inst->lb->clientProposals[i].CommandId, 0,
                                                           nullptr, inst->lb->clientProposals[i].Timestamp);
                // TODO - DONE
                // replyProposeTS(prts, inst->lb->clientProposals[i].Reply);
                // here Reply is the handle of RDMA
                prts->Marshal(inst->lb->clientProposals[i].Conn);
            }
        }

        // TODO: record into stable storage
        // recordInstanceMetadata(inst);
        // sync();   // is this necessary here?
        bcastCommit(pareply->Replica, pareply->Instance, inst->cmds, inst->seq, inst->deps);
    } else if (inst->lb->preAcceptOKs >= group_size / 2) {
        if (!allCommitted)
            weird++;
        slow++;
        inst->status = ACCEPTED;
        bcastAccept(pareply->Replica, pareply->Instance, inst->ballot,
                    int32_t(inst->cmds.size()), inst->seq, inst->deps);
    }
    // TODO: take the slow path if messages are slow to arrive (original lost)
}

void Replica::handlePreAcceptOK(PreAcceptOk * preacok) {
    // TODO
    // LOG("Handling PreAccept OK\n");
    tk_instance * inst = InstanceMatrix[Id][preacok->Instance];

    if (inst == nullptr || inst->status == PREACCEPTED) {
        // we have moved on, this is a delayed reply
        return;
    }

    if (!isInitialBallot(inst->ballot))
        return;

    inst->lb->preAcceptOKs++;

    bool allCommitted = true;
    for (int q = 0; q < group_size; q++) {
        if (inst->lb->committedDeps[q] < inst->lb->originalDeps[q])
            inst->lb->committedDeps[q] = inst->lb->originalDeps[q];
        if (inst->lb->committedDeps[q] < committedUpTo[q])
            inst->lb->committedDeps[q] = committedUpTo[q];
        if (inst->lb->committedDeps[q] < inst->deps[q])
            allCommitted = false;
    }

    // can we commit on the fast path?
    if (inst->lb->preAcceptOKs >= group_size / 2 &&
        inst->lb->allEqual && allCommitted &&
        isInitialBallot(inst->ballot)) {
        happy++;
        InstanceMatrix[Id][preacok->Instance]->status = COMMITTED;
        updateCommitted(Id);
        if (!inst->lb->clientProposals.empty() && !Dreply) {
            // give clients the all clear
            for (int i = 0; i < inst->lb->clientProposals.size(); i++) {
                ProposeReplyTS * prts = new ProposeReplyTS(true, inst->lb->clientProposals[i].CommandId, 0,
                                                           nullptr, inst->lb->clientProposals[i].Timestamp);
                // TODO - DONE
                // replyProposeTS(prts, inst->lb->clientProposals[i].Reply);
                // here Reply is the handle of RDMA
                prts->Marshal(inst->lb->clientProposals[i].Conn);
            }
        }

        // TODO: record into stable storage
        // recordInstanceMetadata(inst);
        // sync();   // is this necessary here?
        bcastCommit(Id, preacok->Instance, inst->cmds, inst->seq, inst->deps);
    } else if (inst->lb->preAcceptOKs >= group_size / 2) {
        if (!allCommitted)
            weird++;
        slow++;
        inst->status = ACCEPTED;
        bcastAccept(Id, preacok->Instance, inst->ballot,
                    int32_t(inst->cmds.size()), inst->seq, inst->deps);
    }
    // TODO: take the slow path if messages are slow to arrive (original lost)
}

/***************************************************************************************
 *                                   PHASE  2                                          *
 **************************************************************************************/

void Replica::handleAccept(Accept * accept) {
    tk_instance * inst = InstanceMatrix[accept->Replica][accept->Instance];
    if (accept->Seq >= maxSeq)
        maxSeq = accept->Seq + 1;

    if (inst != nullptr && (inst->status == COMMITTED || inst->status == EXECUTED))
        return;

    if (accept->Instance >= crtInstance[accept->LeaderId])
        crtInstance[accept->LeaderId] = accept->Instance + 1;

    if (inst != nullptr) {
        if (accept->Ballot < inst->ballot) {
            AcceptReply * acr = new AcceptReply(accept->Replica, accept->Instance, inst->ballot, false);
            replyAccept(accept->LeaderId, acr);
            return;
        }
        inst->status = ACCEPTED;
        inst->seq = accept->Seq;
        inst->deps = accept->Deps;
    } else {
        tk_instance * newInstance = new tk_instance(emptyVec_cmd, accept->Ballot, ACCEPTED,
                                                    accept->Seq, accept->Deps, nullptr);
        InstanceMatrix[accept->Replica][accept->Instance] = newInstance;
        if (accept->Count == 0) {
            // checkpoint, update last checkpoint info
            latestCPReplica = accept->Replica;
            latestCPInstance = accept->Instance;
            clearHashTables();
        }
    }
    // TODO
    // recordInstanceMetadata(InstanceMatrix[accept->Replica][accept->Instance]);
    // sync();

    AcceptReply * acr = new AcceptReply(accept->Replica, accept->Instance, accept->Ballot, true);
    replyAccept(accept->LeaderId, acr);
}

void Replica::handleAcceptReply(AcceptReply * acreply) {
    tk_instance * inst = InstanceMatrix[acreply->Replica][acreply->Instance];

    if (inst == nullptr || inst->status != ACCEPTED) {
        // we have moved on, they are delayed replies, so just ignore
        return;
    }
    if (!acreply->Ok) {
        // TODO: there is probably another active leader (original lost)
        inst->lb->nacks++;
        if (acreply->Ballot > inst->lb->maxRecvBallot)
            inst->lb->maxRecvBallot = acreply->Ballot;
        if (inst->lb->nacks >= group_size / 2) {
            // TODO (original lost)
        }
        return;
    }

    inst->lb->acceptOKs++;
    if (inst->lb->acceptOKs > group_size / 2) {
        InstanceMatrix[acreply->Replica][acreply->Instance]->status = COMMITTED;
        updateCommitted(acreply->Replica);
        if (!inst->lb->clientProposals.empty() && !Dreply) {
            // give clients the all clear
            for (int i = 0; i < inst->lb->clientProposals.size(); i++) {
                ProposeReplyTS * prts = new ProposeReplyTS(true, inst->lb->clientProposals[i].CommandId, 0,
                                                           nullptr, inst->lb->clientProposals[i].Timestamp);
                // TODO - DONE
                // replyProposeTS(prts, inst->lb->clientProposals[i].Reply);
                // here Reply is the handle of RDMA
                // TEST
                prts->Marshal(inst->lb->clientProposals[i].Conn);
            }
        }
        // TODO: record into stable storage
        // recordInstanceMetadata(inst);
        // sync();   // is this necessary here?
        bcastCommit(acreply->Replica, acreply->Instance, inst->cmds, inst->seq, inst->deps);
    }
}

/***************************************************************************************
 *                                 Commit Phase                                        *
 **************************************************************************************/

void Replica::handleCommit(Commit * commit) {
    tk_instance * inst = InstanceMatrix[commit->Replica][commit->Instance];

    if (commit->Seq > maxSeq)
        maxSeq = commit->Seq + 1;
    if (commit->Instance >= crtInstance[commit->Replica])
        crtInstance[commit->Replica] = commit->Instance + 1;

    if (inst != nullptr) {
        if (inst->lb != nullptr && !inst->lb->clientProposals.empty() && commit->Command.empty()) {
            //someone committed a NO-OP, but we have proposals for this instance
            //try in a different instance
            for (Propose p: inst->lb->clientProposals) {
                Propose * pp = new Propose(p);
                pro_mq->put(&pp);
                /* TODO: verify whether this implementation is right, here the original
                  implementation is r.ProposeChan <- p                               */
            }
            delete inst->lb;
            inst->lb = nullptr;
        }
        inst->seq = commit->Seq;
        inst->deps = commit->Deps;
        inst->status = COMMITTED;
    } else {
        tk_instance * newInstance = new tk_instance(commit->Command, 0, COMMITTED,
                                                    commit->Seq, commit->Deps, nullptr);
        InstanceMatrix[commit->Replica][commit->Instance] = newInstance;
        updateConflicts(commit->Command, commit->Replica, commit->Instance, commit->Seq);

        if (commit->Command.empty()) {
            // checkpoint, update last checkpoint info
            latestCPReplica = commit->Replica;
            latestCPInstance = commit->Instance;
            clearHashTables();
        }
    }
    updateCommitted(commit->Replica);
    /* TODO: record into stable storage
     * recordInstanceMetadata(InstanceMatrix[commit->Replica][commit->Instance]);
	 * recordCommands(commit->Command);                      */
}

void Replica::handleCommitShort(CommitShort * commit) {
    tk_instance * inst = InstanceMatrix[commit->Replica][commit->Instance];

    if (commit->Instance >= crtInstance[commit->Replica])
        crtInstance[commit->Replica] = commit->Instance + 1;

    if (inst != nullptr) {
        if (inst->lb != nullptr && !inst->lb->clientProposals.empty()) {
            // try command in a different instance
            for (Propose p: inst->lb->clientProposals) {
                Propose * pp = new Propose(p);
                pro_mq->put(&pp);
                /* TODO: verify whether this implementation is right, here the original
                  implementation is r.ProposeChan <- p                               */
            }
            delete inst->lb;
            inst->lb = nullptr;
        }
        inst->seq = commit->Seq;
        inst->deps = commit->Deps;
        inst->status = COMMITTED;
    } else {
        tk_instance * newInstance = new tk_instance(emptyVec_cmd, 0, COMMITTED,
                                                    commit->Seq, commit->Deps, nullptr);
        InstanceMatrix[commit->Replica][commit->Instance] = newInstance;

        if (commit->Count == 0) {
            // checkpoint, update last checkpoint info
            latestCPReplica = commit->Replica;
            latestCPInstance = commit->Instance;
            clearHashTables();
        }
    }
    updateCommitted(commit->Replica);
    /* TODO: record into stable storage
     * recordInstanceMetadata(InstanceMatrix[commit->Replica][commit->Instance]); */
}

/***************************************************************************************
 *                                 Helper Functions                                    *
 **************************************************************************************/

bool Replica::updateAttributes(std::vector<tk_command> & cmds, int32_t &seq,
                         std::array<int32_t, GROUP_SZIE> & deps,
                         int32_t replica, int32_t instance) {
    bool changed = 0;
    for (int q = 0; q < group_size; q++) {
        if (Id != replica && q == replica)
            continue;
        for (int i = 0; i < cmds.size(); i++) {
            auto p = (conflicts[q]).find(cmds[i].key);
            if (p != conflicts[q].end()) {
                if (p->second > deps[q]) {
                    deps[q] = p->second;
                    if (seq <= InstanceMatrix[q][p->second]->seq) {
                        seq = InstanceMatrix[q][p->second]->seq + 1;
                    }
                    changed = true;
                    break;
                }
            }
        }
    }
    for (int i = 0; i < cmds.size(); i++) {
        auto s = maxSeqPerKey.find(cmds[i].key);
        if (s != maxSeqPerKey.end() && seq <= s->second) {
            changed = true;
            seq = s->second + 1;
        }
    }
    return changed;
}

bool Replica::mergeAttributes(int32_t & seq1, std::array<int32_t, GROUP_SZIE> & deps1,
                              int32_t seq2, std::array<int32_t, GROUP_SZIE> & deps2) {
    bool equal = true;
    if (seq1 != seq2) {
        equal = false;
        if (seq2 > seq1)
            seq1 = seq2;
    }
    for (int q = 0; q < group_size; q++) {
        if (q == Id) {
            continue;
        }
        if (deps1[q] != deps2[q]) {
            equal = false;
            if (deps2[q] > deps1[q])
                deps1[q] = deps2[q];
        }
    }
    return equal;
}

bool Replica::Conflict(tk_command &delta, tk_command &gamma) {
    if (delta.key == gamma.key) {
        if (delta.opcode == PUT || gamma.opcode == PUT)
            return true;
    }
    return false;
}

bool Replica::ConflictBatch(std::vector<tk_command> &batch1,
                            std::vector<tk_command> &batch2) {
    for (int i = 0; i < batch1.size(); i++) {
        for (int j = 0; j < batch2.size(); j++) {
            if (Conflict(batch1[i], batch2[j]))
                return true;
        }
    }
    return false;
}

void Replica::updateConflicts(std::vector<tk_command> &cmds, int32_t replica,
                              int32_t instance, int32_t seq) {
    for (int i = 0; i < cmds.size(); i++) {
        auto d = (conflicts[replica]).find(std::string(cmds[i].key));
        if (d != conflicts[replica].end() && d->second < instance) {
            conflicts[replica][cmds[i].key] = instance;
        } else {
            conflicts[replica][cmds[i].key] = instance;
        }
        auto s = maxSeqPerKey.find(cmds[i].key);
        if (s != maxSeqPerKey.end() && s->second < seq) {
            maxSeqPerKey[cmds[i].key] = seq;
        } else {
            maxSeqPerKey[cmds[i].key] = seq;
        }
    }
}

void Replica::updateCommitted(int32_t replica) {
    while (InstanceMatrix[replica][committedUpTo[replica]+1] != nullptr &&
           (InstanceMatrix[replica][committedUpTo[replica]+1]->status == COMMITTED ||
            InstanceMatrix[replica][committedUpTo[replica]+1]->status == EXECUTED)) {
        committedUpTo[replica]++;
    }
}

void Replica::updatePreferredPeerOrder(std::vector<int32_t> & quorum) {
    std::vector<int32_t> aux((unsigned long)group_size, 0);
    int i = 0;
    for (int32_t p: quorum) {
        if (p == Id)
            continue;
        aux[i++] = p;
    }
    for (int32_t p: PreferredPeerOrder) {
        bool found = false;
        for (int j = 0; j < i; j++) {
            if (aux[j] == p) {
                found = true;
                break;
            }
        }
        if (!found) {
            aux[i++] = p;
        }
    }
    PreferredPeerOrder = aux;
}

void Replica::clearHashTables() {
    for (int q = 0; q < group_size; q++) {
        conflicts[q].clear();
    }
}

int32_t Replica::makeUniqueBallot(int32_t ballot) {
    return (ballot << 4) | Id;
}

int32_t Replica::makeBallotLargerThan(int32_t ballot) {
    return makeUniqueBallot((ballot >> 4) + 1);
}


/*********************************************************************************************
 *                               Inter-Replica communication                                 *
 ********************************************************************************************/

void Replica::connectToPeers(std::vector<std::thread *> & threads) {

    bool done = false;
    std::thread wfpr(waitForPeerConnections, this, &done);
    wfpr.detach();

    //connect to peers
    for (int i = 0; i < Id; i++) {
        bool finish = false;
        while (!finish) {
            // TODO - DONE
            // RDMA_CONNECTION conn == Dial(PeerAddrList[i]);
            // if (conn != ERROR) {
            //     Peers[i] = conn;
            //     finish = true;
            // } else
            //     nano_sleep(1000 * 1000 * 1000);
            // TEST
            RDMA_CONNECTION conn = dialTo(PeerAddrList[i], (uint16_t)PeerPortList[i]);
            if (conn < 0) {
                fprintf(stderr, "Cannot dial to replica %d(%s:%d)\n", i, PeerAddrList[i].c_str(), PeerPortList[i]);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            } else {
                Peers[i] = conn;
                finish = true;
            }
        }
        // TODO - DONE
        // if (RDMA_Write(Peers[i], b) == ERROR) {
        //    LOG("Write Id error!");
        //    continue;
        // }
        // TEST
        int32_t tmp = Id;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (sendData(Peers[i], (char *) &tmp, 4) != 4) {
            fprintf(stderr, "Write Id error!\n");
            continue;
        }
        Alive[i] = true;
        // PeerReaders[i] = NewReader(Peers[i])
        // PeerWriters[i] = NewWriter(Peers[i])
    }

    while (!done);
//    TODO - DONE
//    LOG("Replica id: %d. Done connecting to peers\n", Id);
//    for (int32_t rid = 0; rid < group_size; rid++) {
//        if (rid == Id)
//            continue;
//        _ListenerParam * lpr = new _LIstenerParam(this, rid, reader);
//        go(replicaListener, lpr);
//    }
    // TEST
    fprintf(stdout, "Replica %d, connect to peers: DONE!\n", Id);
    for (int32_t rid = 0; rid < group_size; rid++) {
        if (rid == Id)
            continue;
        threads.push_back(new std::thread(replicaListener, this, rid, Peers[rid]));
        threads.back()->detach();
        fprintf(stdout, "Create listener for replica %d: DONE!\n", rid);
    }
}

void Replica::bcastPreAccept(int32_t replica, int32_t instance, uint32_t ballot,
                             std::vector<tk_command> &cmds, int32_t seq,
                             std::array<int32_t, GROUP_SZIE> & deps) {
    pa.LeaderId = Id; pa.Replica = replica; pa.Ballot = ballot;
    pa.Instance = instance; pa.Command = cmds; pa.Seq = seq; pa.Deps = deps;
    int n = group_size - 1, sent = 0;
    if (Thrifty) {
         n = group_size / 2;
    }

    for (int q = 0; q < group_size - 1; q++) {
        if (!Alive[PreferredPeerOrder[q]])
            continue;
        // TODO - DONE
        // sendPreAccept(PreferredPeerOrder[q], &pa);
        // if this method fails, log failure
        // TEST
        pa.Marshal(PreferredPeerOrder[q]);
        sent++;
        if (sent >= n)
            break;
    }
}

void Replica::bcastCommit(int32_t replica, int32_t instance, std::vector<tk_command> &cmds,
                          int32_t seq, std::array<int32_t, GROUP_SZIE> &deps) {
    cm.LeaderId = Id; cm.Replica = replica; cm.Instance = instance;
    cm.Seq = seq; cm.Command = cmds; cm.Deps = deps;
    cms.LeaderId = Id; cms.Replica = replica; cms.Instance = instance;
    cms.Count = (int32_t)cmds.size(); cms.Seq = seq; cms.Deps = deps;
    int sent = 0;
    for (int q = 0; q < group_size - 1; q++) {
        if (!Alive[PreferredPeerOrder[q]])
            continue;
        if (Thrifty && sent > group_size / 2) {
            // TODO - DONE
            // sendCommit(PreferredPeerOrder[q], &cm);
            // if this or the next method fails, log failure
            // TEST
            cm.Marshal(PreferredPeerOrder[q]);
        } else {
            // TODO - DONE
            // sendCommitShort(PreferredPeerOrder[q], &cms);
            // TEST
            cms.Marshal(PreferredPeerOrder[q]);
            sent++;
        }
    }
}

void Replica::bcastAccept(int32_t replica, int32_t instance, int32_t ballot, int32_t count,
                          int32_t seq, std::array<int32_t, GROUP_SZIE> &deps) {
    ea.LeaderId = Id; ea.Replica = replica; ea.Instance = instance; ea.Ballot = ballot;
    ea.Count = count; ea.Seq = seq; ea.Deps = deps;
    int n = group_size - 1;
    if (Thrifty)
        n = group_size / 2;

    int sent = 0;
    for (int q = 0; q < group_size - 1; q++) {
        if (!Alive[PreferredPeerOrder[q]])
            continue;
        // TODO - DONE
        // sendAccept(PreferredPeerOrder[q], &ea);
        // TEST
        ea.Marshal(PreferredPeerOrder[q]);
        sent++;
        if (sent >= n)
            break;
    }
}

void Replica::bcastPrepare(int32_t replica, int32_t instance, int32_t ballot) {
    pr.LeaderId = Id; pr.Replica = replica; pr.Instance = instance; pr.Ballot = ballot;
    int n = group_size - 1;
    if (Thrifty)
        n = group_size / 2;
    int q = Id, sent = 0;
    while (sent < n) {
        q = (q + 1) % int32_t(group_size);
        if (q == Id) {
            // TODO - DONE
            // LOG("Not enough replicas alive");
            // TEST
            fprintf(stderr, "Not Enough Replicas Alive!\n");
            break;
        }
        if (!Alive[q])
            continue;
        // TODO - DONE
        // sendPrepare(q, &pr);
        // TEST
        pr.Marshal(Peers[q]);
        sent++;
    }
}

void Replica::bcastTryPreAccept(int32_t replica, int32_t instance, int32_t ballot,
                                std::vector<tk_command> &cmds,
                                int32_t seq, std::array<int32_t, GROUP_SZIE> &deps) {
    tpa.LeaderId = Id; tpa.Replica = replica; tpa.Instance = instance;
    tpa.Ballot = ballot; tpa.Seq = seq; tpa.Command = cmds; tpa.Deps = deps;
    for (int q = 0; q < group_size - 1; q++) {
        if (q == Id)
            continue;
        if (!Alive[q])
            continue;
        // TODO - DONE
        // sendTryPreAccept(q, &tpa);
        // TEST
        tpa.Marshal(Peers[q]);
    }
}

/*********************************************************************************************
 *                                     Recovery Actions                                      *
 ********************************************************************************************/

void Replica::startRecoveryForInstance(int32_t replica, int32_t instance) {
    std::array<int32_t, GROUP_SZIE> nildeps;

    if (InstanceMatrix[replica][instance] == nullptr) {
        tk_instance * newInstance = new tk_instance(emptyVec_cmd, 0, NONE,
                                                    0, nildeps, nullptr);
        InstanceMatrix[replica][instance] = newInstance;
    }

    tk_instance * inst = InstanceMatrix[replica][instance];
    if (inst->lb == nullptr) {
        lb_t * nlb = new lb_t(emptyVec_pro, -1, 0, false, 0, 0, 0, nildeps,
                              emptyVec_int, nullptr, true, false, emptyVec_bool, 0);
        inst->lb = nlb;
    } else {
        lb_t * nlb = new lb_t(inst->lb->clientProposals, -1, 0, false, 0, 0, 0, nildeps,
                              emptyVec_int, nullptr, true, false, emptyVec_bool, 0);
        inst->lb = nlb;
    }
    if (inst->status == ACCEPTED) {
        RecoverInstance * rvi = new RecoverInstance(false, inst->status, inst->seq,
                                                    0, inst->deps, inst->cmds);
        inst->lb->recoveryInst = rvi;
        inst->lb->maxRecvBallot = inst->ballot;
    } else if (inst->status == PREACCEPTED) {
        RecoverInstance * rvi = new RecoverInstance(Id == replica, inst->status, inst->seq,
                                                    1, inst->deps, inst->cmds);
        inst->lb->recoveryInst = rvi;
    }

    // compute large ballot
    inst->ballot = makeBallotLargerThan(inst->ballot);
    bcastPrepare(replica, instance, inst->ballot);
}

void Replica::handlePrepare(Prepare *prepare) {
    tk_instance * inst = InstanceMatrix[prepare->Replica][prepare->Instance];
    PrepareReply * preply;
    std::array<int32_t, GROUP_SZIE> nildeps;

    if (inst == nullptr) {
        tk_instance * newInstance = new tk_instance(emptyVec_cmd, prepare->Ballot,
                                                    NONE, 0, nildeps, nullptr);
        preply = new PrepareReply(Id, prepare->Replica, prepare->Instance, true, -1,
                                  NONE, emptyVec_cmd, -1, nildeps);
    } else {
        bool ok = true;
        if (prepare->Ballot < inst->ballot)
            ok = false;
        else
            inst->ballot = prepare->Ballot;
        preply = new PrepareReply(Id, prepare->Replica, prepare->Instance, ok, inst->ballot,
                                  inst->status, inst->cmds, inst->seq, inst->deps);
    }
    replyPrepare(prepare->LeaderId, preply);
}

void Replica::handlePrepareReply(PrepareReply * preply) {
    tk_instance * inst = InstanceMatrix[preply->Replica][preply->Instance];

    if (inst == nullptr || inst->lb == nullptr || !inst->lb->preparing) {
        // we've moved on -- these are delayed replies, so just ignore
        // TODO: should replies for non-current ballots be ignored? (original lost)
        return;
    }

    if (!preply->Ok) {
        // TODO: there is probably another active leader, back off and retry later (original lost)
        inst->lb->nacks++;
        return;
    }

    //Got an ACK (preply.OK == TRUE)

    inst->lb->prepareOKs++;

    if (preply->Status == COMMITTED || preply->Status == EXECUTED) {
        tk_instance * newInstance = new tk_instance(preply->Command, inst->ballot, COMMITTED,
                                             preply->Seq, preply->Deps, nullptr);
        InstanceMatrix[preply->Replica][preply->Instance] = newInstance;
        bcastCommit(preply->Replica, preply->Instance, inst->cmds, preply->Seq, preply->Deps);
        //TODO: check if we should send notifications to clients (original lost)
        return;
    }

    if (preply->Status == ACCEPTED) {
        if (inst->lb->recoveryInst == nullptr || inst->lb->maxRecvBallot < preply->Ballot) {
            RecoverInstance * rvi = new RecoverInstance(false, preply->Status, 0, preply->Seq,
                                                        preply->Deps, preply->Command);
            inst->lb->recoveryInst = rvi;
            inst->lb->maxRecvBallot = preply->Ballot;
        }
    }

    if ((preply->Status == PREACCEPTED || preply->Status == PREACCEPTED_EQ) &&
        (inst->lb->recoveryInst == nullptr || inst->lb->recoveryInst->status < ACCEPTED)) {
        if (inst->lb->recoveryInst == nullptr) {
            RecoverInstance * rvi = new RecoverInstance(false, preply->Status, 1, preply->Seq,
                                                         preply->Deps, preply->Command);
            inst->lb->recoveryInst = rvi;
        } else if (preply->Seq == inst->seq && equal(preply->Deps, inst->deps)) {
            inst->lb->recoveryInst->preAcceptCount++;
        } else if (preply->Status == PREACCEPTED_EQ) {
            // If we get different ordering attributes from pre-acceptors, we must go with the ones
            // that agreed with the initial command leader (in case we do not use Thrifty).
            // This is safe if we use thrifty, although we can also safely start phase 1 in that case.
            RecoverInstance * rvi = new RecoverInstance(false, preply->Status, 1, preply->Seq,
                                                        preply->Deps, preply->Command);
            inst->lb->recoveryInst = rvi;
        }
        if (preply->AcceptorId == preply->Replica) {
            //if the reply is from the initial command leader, then it's safe to restart phase 1
            inst->lb->recoveryInst->leaderResponded = true;
            return;
        }
    }

    if (inst->lb->prepareOKs < group_size / 2)
        return;

    //Received Prepare replies from a majority

    RecoverInstance * ir = inst->lb->recoveryInst;

    if (ir != nullptr) {
        //at least one replica has (pre-)accepted this instance
        if (ir->status == ACCEPTED ||
            (!ir->leaderResponded && ir->preAcceptCount >= group_size / 2 &&
             (Thrifty || ir->status == PREACCEPTED_EQ))) {
            //safe to go to Accept phase
            inst->cmds = ir->cmds;
            inst->seq = ir->seq;
            inst->deps = ir->deps;
            inst->status = ACCEPTED;
            inst->lb->preparing = false;
            bcastAccept(preply->Replica, preply->Instance, inst->ballot,
                        int32_t(inst->cmds.size()), inst->seq, inst->deps);
        } else if (!ir->leaderResponded && ir->preAcceptCount >= (group_size/2 + 1)/2) {
            //send TryPreAccepts
            //but first try to pre-accept on the local replica
            inst->lb->preAcceptOKs = 0;
            inst->lb->nacks = 0;
            inst->lb->possibleQuorum = std::vector<bool>((unsigned long)group_size, false);
            for (int q = 0; q < group_size; q++) {
                inst->lb->possibleQuorum[q] = true;
            }
            int q, i;
            if (findPreAcceptConflicts(ir->cmds, preply->Replica, preply->Instance, ir->seq, ir->deps, &q, &i)) {
                if (q == -1 || i == -1)
                    return;
                if (InstanceMatrix[q][i]->status >= COMMITTED) {
                    //start Phase1 in the initial leader's instance
                    startPhase1(preply->Replica, preply->Instance, inst->ballot,
                                inst->lb->clientProposals, ir->cmds, ir->cmds.size());
                    return;
                } else {
                    inst->lb->nacks = 1;
                    inst->lb->possibleQuorum[Id] = false;
                }
            } else {
                inst->cmds = ir->cmds;
                inst->seq = ir->seq;
                inst->deps = ir->deps;
                inst->status = PREACCEPTED;
                inst->lb->preAcceptOKs = 1;
            }
            inst->lb->preparing = false;
            inst->lb->tryingToPreAccept = true;
            bcastTryPreAccept(preply->Replica, preply->Instance, inst->ballot,
                              inst->cmds, inst->seq, inst->deps);
        } else {
            //start Phase1 in the initial leader's instance
            inst->lb->preparing = false;
            startPhase1(preply->Replica, preply->Instance, inst->ballot,
                        inst->lb->clientProposals, ir->cmds, ir->cmds.size());
        }
    } else {
        //try to finalize instance by proposing NO-OP
        std::array<int32_t, GROUP_SZIE> noop_deps;
        // commands that depended on this instance must look at all previous instances
        noop_deps[preply->Replica] = preply->Instance - 1;
        inst->lb->preparing = false;
        tk_instance * newInstance = new tk_instance(emptyVec_cmd, inst->ballot, ACCEPTED, 0, noop_deps, inst->lb);
        InstanceMatrix[preply->Replica][preply->Instance] = newInstance;
        bcastAccept(preply->Replica, preply->Instance, inst->ballot, 0, 0, noop_deps);
    }
}

void Replica::handleTryPreAccept(TryPreAccept * tpap) {
    tk_instance * inst = InstanceMatrix[tpap->Replica][tpap->Instance];

    if (inst != nullptr && inst->ballot > tpap->Ballot) {
        // ballot number too small
        TryPreAcceptReply * ntpap = new TryPreAcceptReply(Id, tpap->Replica, tpap->Instance, inst->ballot,
                                                          false, tpap->Replica, tpap->Instance, inst->status);
        replyTryPreAccept(tpap->LeaderId, ntpap);
    }
    int confRep, confInst;
    if (findPreAcceptConflicts(tpap->Command, tpap->Replica, tpap->Instance,
                               tpap->Seq, tpap->Deps, &confRep, &confInst)) {
        // there is a conflict, can't pre-accept
        TryPreAcceptReply * ntpap = new TryPreAcceptReply(Id, tpap->Replica, tpap->Instance, inst->ballot,
                                                          false, confRep, confInst,
                                                          InstanceMatrix[confRep][confInst]->status);
        replyTryPreAccept(tpap->LeaderId, ntpap);
    } else {
        // can pre-accept
        if (tpap->Instance >= crtInstance[tpap->Replica]) {
            crtInstance[tpap->Replica] = tpap->Instance + 1;
        }
        if (inst != nullptr) {
            inst->cmds = tpap->Command;
            inst->deps = tpap->Deps;
            inst->seq = tpap->Seq;
            inst->status = PREACCEPTED;
            inst->ballot = tpap->Ballot;
        } else {
            tk_instance * newInstance = new tk_instance(tpap->Command, tpap->Ballot, PREACCEPTED,
                                                        tpap->Seq, tpap->Deps, nullptr);
            InstanceMatrix[tpap->Replica][tpap->Instance] = newInstance;
        }
        TryPreAcceptReply * ntpap = new TryPreAcceptReply(Id, tpap->Replica, tpap->Instance,
                                                          inst->ballot, true, 0, 0, NONE);
        replyTryPreAccept(tpap->LeaderId, ntpap);
    }
}

void Replica::handleTryPreAcceptReply(TryPreAcceptReply * tpar) {
    tk_instance * inst = InstanceMatrix[tpar->Replica][tpar->Instance];
    if (inst == nullptr || inst->lb == nullptr ||
        !inst->lb->tryingToPreAccept ||
        inst->lb->recoveryInst == nullptr) {
            return;
        }

    RecoverInstance * ir = inst->lb->recoveryInst;

    if (tpar->Ok) {
        inst->lb->preAcceptOKs++;
        inst->lb->tpaOks++;
        if (inst->lb->preAcceptOKs >= group_size / 2) {
            //it's safe to start Accept phase
            inst->cmds = ir->cmds;
            inst->seq = ir->seq;
            inst->deps = ir->deps;
            inst->status = ACCEPTED;
            inst->lb->tryingToPreAccept = false;
            inst->lb->acceptOKs = 0;
            bcastAccept(tpar->Replica, tpar->Instance, inst->ballot, int32_t(inst->cmds.size()), inst->seq, inst->deps);
            return;
        }
    } else {
        inst->lb->nacks++;
        if (tpar->Ballot > inst->ballot) {
            // TODO: retry with higher ballot (original lost)
            return;
        }
        inst->lb->tpaOks++;
        if (tpar->ConflictReplica == tpar->Replica && tpar->ConflictInstance == tpar->Instance) {
            // TODO: re-run prepare (original lost)
            inst->lb->tryingToPreAccept = false;
            return;
        }
        inst->lb->possibleQuorum[tpar->AcceptorId] = false;
        inst->lb->possibleQuorum[tpar->ConflictReplica] = false;

        int notInQuorum = 0;
        for (int q = 0; q < group_size; q++) {
            if (!inst->lb->possibleQuorum[tpar->AcceptorId]) {
                notInQuorum++;
            }
        }
        if (tpar->ConflictStatus >= COMMITTED || notInQuorum > group_size / 2) {
            // abandon recovery, restart from phase 1
            inst->lb->tryingToPreAccept = false;
            startPhase1(tpar->Replica, tpar->Instance, inst->ballot,
                        inst->lb->clientProposals, ir->cmds, ir->cmds.size());
        }
        if (notInQuorum == group_size / 2) {
            // this is to prevent defer cycles
            int32_t dq, di;
            if (deferredByInstance(tpar->Replica, tpar->Instance, &dq, &di)) {
                if (inst->lb->possibleQuorum[dq]) {
                    // an instance whose leader must have been in this instance's quorum has been deferred for
                    // this instance => contradiction abandon recovery, restart from phase 1
                    inst->lb->tryingToPreAccept = false;
                    startPhase1(tpar->Replica, tpar->Instance, inst->ballot,
                                inst->lb->clientProposals, ir->cmds, ir->cmds.size());
                }
            }
        }
        if (inst->lb->tpaOks >= group_size / 2) {
            //defer recovery and update deferred information
            updateDeferred(tpar->Replica, tpar->Instance, tpar->ConflictReplica, tpar->ConflictInstance);
            inst->lb->tryingToPreAccept = false;
        }
    }
}

bool Replica::findPreAcceptConflicts(std::vector<tk_command> &cmds, int32_t replica,
                                     int32_t instance, int32_t seq,
                                     std::array<int32_t, GROUP_SZIE> &deps,
                                     int *qr, int *ir) {
    tk_instance * inst = InstanceMatrix[replica][instance];
    if (inst != nullptr && inst->cmds.size() > 0) {
        if (inst->status >= ACCEPTED) {
            // already ACCEPTED or COMMITTED
            // we consider this a conflict because we shouldn't regress to PRE-ACCEPTED
            *qr = replica; *ir = instance;
            return true;
        }
        if (inst->seq == tpa.Seq && equal(inst->deps, tpa.Deps)) {
            // already PRE-ACCEPTED, no point looking for conflicts again
            *qr = replica; *ir = instance;
            return false;
        }
    }
    for (int q = 0; q < group_size; q++) {
        for (int32_t i = executeUpTo[q]; i < crtInstance[q]; i++) {
            if (replica == q && instance == i) {
                // no point checking past instance in replica's row, since replica would have
                // set the dependencies correctly for anything started after instance
                break;
            }
            if (i == deps[q]) {
                //the instance cannot be a dependency for itself
                continue;
            }
            tk_instance * _inst = InstanceMatrix[q][i];
            if (_inst == nullptr || _inst->cmds.empty()) {
                continue;
            }
            if (_inst->deps[replica] >= instance) {
                // instance q.i depends on instance replica.instance, it is not a conflict
                continue;
            }
            if (ConflictBatch(_inst->cmds, cmds)) {
                if (i > deps[q] ||
                    (i < deps[q] && _inst->seq >= seq && (q != replica || _inst->status > PREACCEPTED_EQ))) {
                    // this is a conflict


                    *qr = q; *ir = i;
                    return true;
                }
            }
        }
    }
    *qr = -1; *ir = -1;
    return false;
}

/*******************************************************************************************
 *                               Reply Methods                                             *
 ******************************************************************************************/

void Replica::replyPrepare(int32_t LeaderId, PrepareReply * preply) {
    // TODO - DONE
    preply->Marshal(Peers[LeaderId]);
}

void Replica::replyPreAccept(int32_t LeaderId, PreAcceptReply * preacply) {
    // TODO - DONE
    preacply->Marshal(Peers[LeaderId]);
}

void Replica::replyAccept(int32_t LeaderId, AcceptReply * acr) {
    // TODO - DONE
    acr->Marshal(Peers[LeaderId]);
}

void Replica::replyTryPreAccept(int32_t LeaderId, TryPreAcceptReply * ntpap) {
    // TODO - DONE
    ntpap->Marshal(Peers[LeaderId]);
}

void Replica::sendBeacon(int32_t peerId) {
    // TODO - DONE
    // RDMA_Handler w = peerWriters[peerId];
    Beacon_msg * beacon = new Beacon_msg(Id, CPUTicks());
    // beacon.Marshal(w);
    // w.Flush();
    // TEST
    beacon->Marshal(Peers[peerId]);
}

void Replica::replyBeacon(Beacon_msg * beacon) {
    // TODO - DONE
    // RDMA_Handler w = peerWriters[beacon->Rid];
    Beacon_msg_reply * rb = new Beacon_msg_reply(beacon->timestamp);
    // rb.Marshal(w);
    // w.Flush();
    // TEST
    rb->Marshal(Peers[beacon->Rid]);
}

/*******************************************************************************************
*                                Helper non-class Functions                                *
*******************************************************************************************/

void updateDeferred(int32_t dr, int32_t di, int32_t r, int32_t i) {
    uint64_t daux = (((uint64_t)dr) << 32) | ((uint64_t)di);
    uint64_t  aux = (((uint64_t) r) << 32) | ((uint64_t) i);
    deferMap[aux] = daux;
}

bool deferredByInstance(int32_t q, int32_t i, int32_t * dq, int32_t *di) {
    uint64_t aux = (((uint64_t)q) << 32) | ((uint64_t)i);
    auto daux = deferMap.find(aux);
    if (daux == deferMap.end()) {
        *dq = 0;
        *di = 0;
        return false;
    }
    *dq = (int32_t)(daux->second >> 32);
    *di = (int32_t)(daux->second);
    return true;
}

uint64_t CPUTicks() {
    clock_t t = clock();
    return (uint64_t)t;
}

void LEPutUint32(char * buf, int32_t v) {
    buf[0] = (char)v;
    buf[1] = (char)(v >> 8);
    buf[2] = (char)(v >> 16);
    buf[3] = (char)(v >> 24);
}

void msgDispatcher(Replica * r, int32_t rid, RDMA_CONNECTION conn, TYPE msgType) {
    if (msgType == PREACCEPT) {
        PreAccept *pre_accept = new PreAccept();
        pre_accept->Unmarshal(conn);
        r->mq->put(&pre_accept);
    } else if (msgType == PREACCEPT_OK) {
        PreAcceptOk *pre_accept_ok = new PreAcceptOk();
        pre_accept_ok->Unmarshal(conn);
        r->mq->put(&pre_accept_ok);
    } else if (msgType == PREACCEPT_REPLY) {
        PreAcceptReply *pre_accept_reply = new PreAcceptReply();
        pre_accept_reply->Unmarshal(conn);
        r->mq->put(&pre_accept_reply);
    } else if (msgType == PREPARE) {
        Prepare *prepare = new Prepare();
        prepare->Unmarshal(conn);
        r->mq->put(&prepare);
    } else if (msgType == PREPARE_REPLY) {
        PrepareReply *prepare_reply = new PrepareReply();
        prepare_reply->Unmarshal(conn);
        r->mq->put(&prepare_reply);
    } else if (msgType == ACCEPT) {
        Accept *accept = new Accept();
        accept->Unmarshal(conn);
        r->mq->put(&accept);
    } else if (msgType == ACCEPT_REPLY) {
        AcceptReply *accept_reply = new AcceptReply();
        accept_reply->Unmarshal(conn);
        r->mq->put(&accept_reply);
    } else if (msgType == COMMIT) {
        Commit *commit = new Commit();
        commit->Unmarshal(conn);
        r->mq->put(&commit);
    } else if (msgType == COMMIT_SHORT) {
        CommitShort *commit_short = new CommitShort();
        commit_short->Unmarshal(conn);
        r->mq->put(&commit_short);
    } else if (msgType == TRY_PREACCEPT) {
        TryPreAccept *try_pre_accept = new TryPreAccept();
        try_pre_accept->Unmarshal(conn);
        r->mq->put(&try_pre_accept);
    } else if (msgType == TRY_PREACCEPT_REPLY) {
        TryPreAcceptReply *try_pre_accept_reply = new TryPreAcceptReply();
        try_pre_accept_reply->Unmarshal(conn);
        r->mq->put(&try_pre_accept_reply);
    } else {
        fprintf(stderr, "Receive Unknown MsgType: %d\n", msgType);
    }
}