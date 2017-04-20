//
// Created by jingle on 17-3-22.
//

#ifndef TKDATABASE_TK_ELOG_H
#define TKDATABASE_TK_ELOG_H

#include <stdint.h>

#define PUT 1
#define GET  2

#define EXECUTED_MASK 0x01
#define USED_MASK 0x02
#define COMMITTED_MASK 0x04

struct tk_command{
    uint8_t opcode;
    char* key;
    char* val;
};

typedef struct tk_command tk_command_t;


struct LeaderBookkeeping {
    /*
     * to be considered
     * maintain the client proposals related with this instance
     * Propose * clientProposals;
     * uint32_t maxRecvBallot;
     * ...
     */
};

typedef struct LeaderBookkeeping lb_t;

struct tk_instance {

    tk_command_t * cmds;
    unsigned int   cmds_count;
    unsigned int * deps;
    unsigned int   seq;
    uint8_t        flag;
    /*
     * flag & EXECUTED_MASK = executed
     * flag & USED_MASK = used
     * flag & COMMITTED_MASK = committed
     */

    // dfn and low: for tarjan algorithm in execution loop
    unsigned short dfn;
    unsigned short low;

    lb_t * lb;
};

typedef struct tk_instance tk_instance_t;







#endif //TKDATABASE_TK_ELOG_H
