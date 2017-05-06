//
// Created by forec on 17-5-4.
//

#ifndef TKDATABASE_TK_COMMAND_H
#define TKDATABASE_TK_COMMAND_H

#include <stdint.h>
#include <string>

struct tk_command{
    OP opcode;
    std::string key;
    char* val;
};

#endif //TKDATABASE_TK_COMMAND_H
