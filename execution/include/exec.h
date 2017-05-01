//
// Created by forec on 17-4-20.
//

#ifndef TKDATABASE_EXEC_H
#define TKDATABASE_EXEC_H

#include "../../consensus/include/tk_elog.h"
#include "../../consensus/include/replica.h"
#include "../../utils/include/utils.h"
#include <stdint.h>
#include <stdlib.h>

#define STACK instance_stack
#define SLEEP_TIME_NS 1000000
#define COMMIT_GRACE_PERIOD 10*SLEEP_TIME_NS

extern unsigned int instance_stack_size;
extern tk_instance ** instance_stack;
extern unsigned int top;

int cmp_instance(const void *p1, const void *p2);

bool strong_connect(Replica * r, tk_instance * v, unsigned short * index);

bool find_SCC(Replica * r, tk_instance * v);

bool execute_instance(Replica * r, int replica, int instance);

char * execute_command(tk_command * c, Tkdatabase_t * st);

void * execute_thread_t(void * arg);

#endif //TKDATABASE_EXEC_H
