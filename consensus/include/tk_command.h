//
// Created by forec on 17-5-4.
//

#ifndef TKDATABASE_TK_COMMAND_H
#define TKDATABASE_TK_COMMAND_H

#include <stdint.h>
#include <string>
#include "../../utils/include/communication.h"

struct tk_command{
    OP opcode;
    std::string key;
    int32_t valSize;
    char * val;
    tk_command() {
        opcode = GET;
        key.clear();
        valSize = 0;
        val = nullptr;
    }
    tk_command(OP _op, const std::string & _k, int32_t _vs, char * _v) :
        opcode(_op), key(_k), valSize(_vs), val(_v) {};
    void print() {
        fprintf(stdout, "OP = %d, key = %s, valSize = %d, val = %x\n",
                opcode, key.c_str(), valSize, val);
    }
    bool Unmarshal(int sock) {
        char buf[16] = {0};
        buf[15] = 0;
        uint8_t tmp;
        int32_t keyLen;
        readUntil(sock, (char *)&tmp, 1);
        opcode = (OP)tmp;
        readUntil(sock, (char *) &keyLen, 4);
        key.clear();
        while (keyLen > 0) {
            if (keyLen <= 15) {
                readUntil(sock, buf, keyLen);
                buf[keyLen] = '\0';
            }
            else
                readUntil(sock, buf, 15);
            key += std::string(buf);
            keyLen = keyLen >= 15 ? keyLen - 15 : 0;
        }
        uint32_t valLen;
        readUntil(sock, (char *) & valLen, 4);
        valSize = valLen;
        if (val)
            delete [] val;
        val = new char[valSize];
        readUntil(sock, val, valSize);
        return true;
    }
    void Marshal(int sock) {
        char buf[128];
        uint8_t tmp = (uint8_t)opcode;
        sendData(sock, (char *) &tmp, 1);
        int32_t keyLen = (int32_t)key.length(), sentKeyLen = 0;
        sendData(sock, (char *) &keyLen, 4);
        while (keyLen > 0) {
            if (keyLen > 128) {
                memcpy(buf, key.c_str() + sentKeyLen, 128);
                sentKeyLen += 128;
                keyLen -= 128;
                sendData(sock, buf, 128);
            } else {
                memcpy(buf, key.c_str() + sentKeyLen, (size_t)keyLen);
                sendData(sock, buf, (size_t)keyLen);
                break;
            }
        }
        sendData(sock, (char *) &valSize, 4);
        sendData(sock, val, valSize);
    }
};

#endif //TKDATABASE_TK_COMMAND_H
