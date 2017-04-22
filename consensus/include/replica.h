//
// Created by forec on 17-4-22.
//

#ifndef TKDATABASE_REPLICA_H
#define TKDATABASE_REPLICA_H


#include "../include/tk_elog.h"
#include "../include/tk_server.h"
// #include "chan/chan.h"


int verify_param(replica_server_param_t* param);
int init_replica_server(replica_server_param_t* param);
int start_replica_server(replica_server_param_t *param);

#endif //TKDATABASE_REPLICA_H
