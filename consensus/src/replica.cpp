//
// Created by jingle on 17-4-12.
//
#include "../include/replica.h"


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


bool Replica::run() {

    init();

    // TODO: 1. event loop; -- To process the message from servers each other, get msgs from a channel;

    //TODO:  2. excution Loop; -- periodically to apply cmds. build the KV;

    //TODO:  3. propose Loop;  -- client raise a proposal, when server receives a cmd, process it, and put it in a Propose channel;

    //TODO:  4. Timeout Loop;  -- check timeout,

    //TODO:  network start;    --  connect peer and peer, and process the message.

    //coroutine method;
    return true;
}