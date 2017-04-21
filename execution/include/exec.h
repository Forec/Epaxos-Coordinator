//
// Created by forec on 17-4-20.
//

#ifndef TKDATABASE_EXEC_H
#define TKDATABASE_EXEC_H

#include "tk_elog.h"
#include "tk_server.h"
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

unsigned int instance_stack_size = 100;
static tk_instance_t ** instance_stack;
static unsigned int top;

#define STACK instance_stack

void nano_sleep(unsigned int ns);

int cmp_instance(const void *p1, const void *p2);

static uint8_t strong_connect(replica_server_param_t * r,
                              tk_instance_t * v,
                              unsigned short * index);

static uint8_t find_SCC(replica_server_param_t * r,
                        tk_instance_t * v);

static uint8_t execute_instance(replica_server_param_t * r,
                               int replica,
                               int instance);

static char * execute_command(tk_command_t c,
                              Tkdatabase_t * st);
#endif //TKDATABASE_EXEC_H
