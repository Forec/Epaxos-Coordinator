//
// Created by jingle on 17-4-12.
//

#include "../include/tk_server.h"
#include "chan/chan.h"



int verify_param(replica_server_param_t* param)
{
    int ret  = 0;
    if(param->group_size == 0){
        param->group_size = GROUP_SZIE;
    }
    if(param->group_size % 2 == 0){
        error_exit(0, param->log_fp, "Group size must be odd !");
        ret = 1;
    }

    if(param->checkpoint_cycle == 0){
        param->checkpoint_cycle = CHECKPOINT_CYCLE;
    }
    if(param->addrs == NULL){
        param->addrs = (char **)malloc(param->group_size * sizeof(char*));
        int i = 0;
        for(i = 0; i < param-> group_size; i++){
            char* addr;
            int len =0;
            len = sprintf(addr, "localhost:%d", PORT + i);
            if(len != 0){
                *(param->addrs + i) = addr;
            }
        }
    }

    return ret;
}


int init_replica_server(replica_server_param_t* param)
{
    int ret = verify_param(param);

    if(ret){
        printf("init failed, more detail to see the log!\n");
        return ret;
    }

    if(strcmp(param->string_path, "") == 1){

        sprintf(param->string_path,"%s-%d", "/tmp/test", param->replicaId);
    }
    param->InstanceMatrix = (tk_instance_t**) malloc(param->group_size * sizeof(tk_instance_t *));
    param->MaxInstanceNum = (uint64_t *) malloc(param->group_size * sizeof(uint64_t));
    param->excuteupto = (uint64_t *) malloc(param->group_size * sizeof (uint64_t));
    for(uint8_t i = 0; i < param->group_size; i++){
        param->InstanceMatrix[i] = (tk_instance_t *)malloc(1024 * 1024 * sizeof(tk_instance_t));
        param->MaxInstanceNum[i] = 0;
        param->excuteupto[i] = 0;
    }

    if(param->restore){
        //TODO: recovery from log file;
    }

//    chan_init(1000);

    return ret;
}


int start_replica_server(replica_server_param_t *param)
{

    init_replica_server(param);

    // TODO: 1. event loop; -- To process the message from servers each other, get msgs from a channel;

    //TODO:  2. excution Loop; -- periodically to apply cmds. build the KV;

    //TODO:  3. propose Loop;  -- client raise a proposal, when server receives a cmd, process it, and put it in a Propose channel;

    //TODO:  4. Timeout Loop;  -- check timeout,

    //TODO:  network start;    --  connect peer and peer, and process the message.

    //coroutine method;




}