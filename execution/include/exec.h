//
// Created by forec on 17-4-20.
//

#ifndef TKDATABASE_EXEC_H
#define TKDATABASE_EXEC_H

#include "../../consensus/include/tk_elog.h"
#include "../../consensus/include/tk_server.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>


#define STACK instance_stack
#define SLEEP_TIME_NS 1000000
#define COMMIT_GRACE_PERIOD 10*SLEEP_TIME_NS

extern unsigned int instance_stack_size;
extern tk_instance_t ** instance_stack;
extern unsigned int top;

void nano_sleep(unsigned int ns);

int cmp_instance(const void *p1, const void *p2);

uint8_t strong_connect(replica_server_param_t * r,
                       tk_instance_t * v,
                       unsigned short * index);

uint8_t find_SCC(replica_server_param_t * r,
                 tk_instance_t * v);

uint8_t execute_instance(replica_server_param_t * r,
                         int replica,
                         uint64_t instance);

char * execute_command(tk_command_t * c,
                       Tkdatabase_t * st);

void execute_thread(replica_server_param_t * r);

#endif //TKDATABASE_EXEC_H