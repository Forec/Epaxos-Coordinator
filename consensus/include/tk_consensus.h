//
// Created by jingle on 17-2-14.
//

#include <debug>

#ifndef TKDATABASE_TK_CONSENSUS_H
#define TKDATABASE_TK_CONSENSUS_H


/*SM types */

#define CLT_NULL 1
#define CLT_KVS 2
#define CLT_FS 3

/*For immediate event scheduling */
#define NOW 0.000000001

#define MAX_CLIENT_COUNT 64
#define MAX_SERVER_COUNT 13

#define PAGE_SIZE 4096


/**
 *  UD message types
 */
#define MSG_NONE 0
#define MSG_ERROR 13
/* Initialization messages */
#define RC_SYN      1
#define RC_SYNACK   2
#define RC_ACK      3
/* Client SM messages */
#define CSM_READ    201
#define CSM_WRITE   202
#define CSM_REPLY   203
/* Config messages */
#define JOIN        211
#define DOWNSIZE    213
#define CFG_REPLY   214
/* LOGGP messages */
#define LOGGP_UD    55





#endif //TKDATABASE_TK_CONSENSUS_H


