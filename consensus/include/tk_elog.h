//
// Created by jingle on 17-3-22.
//

#ifndef TKDATABASE_TK_ELOG_H
#define TKDATABASE_TK_ELOG_H

#include <stdint.h>

#define PUT 1
#define GET  2


struct tk_command{
    uint8_t opcode;
    char* key;
    char* val;
};

typedef struct tk_command tk_command_t;




struct tk_instance {

    tk_command_t  cmd;
    char **  deps;
    uint8_t  status;
    bool     excuted;

};

typedef struct tk_instance tk_instance_t;







#endif //TKDATABASE_TK_ELOG_H
