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
    uint32_t valSize;
    char * val;
    tk_command() {
        opcode = GET;
        key.clear();
        valSize = 0;
        val = nullptr;
    }
    bool Unmarshal(int sock) {
        char buf[16] = {0};
        readUntil(sock, buf, 1);
        opcode = (OP)*(uint8_t *) buf;
        readUntil(sock, buf, 4);
        int keyLen = *(uint32_t *) buf;
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
        readUntil(sock, buf, 4);
        uint32_t valLen = *(uint32_t *) buf;
        valSize = valLen;
        if (val)
            delete [] val;
        val = new char[valLen];
        int received = 0;
        while (valLen > 0) {
            if (valLen <= 16)
                readUntil(sock, val + received, keyLen);
            else
                readUntil(sock, val + received, 16);
            valLen = valLen > 16 ? valLen - 16 : 0;
        }
        return true;
    }
    void Marshal(int sock) {
        char buf[128];
        buf[0] = (char) opcode;
        sendData(sock, buf, 1);
        int keyLen = key.length() + 1, sentKeyLen = 0;
        while (keyLen > 0) {
            if (keyLen > 128) {
                memcpy(buf, key.c_str() + sentKeyLen, 128);
                sentKeyLen += 128;
                keyLen -= 128;
                sendData(sock, buf, 128);
            } else {
                memcpy(buf, key.c_str() + sentKeyLen, keyLen);
                sendData(sock, buf, keyLen);
                break;
            }
        }
        buf[0] = '\0';
        memcpy(buf + 1, &valSize, 4);
        sendData(sock, buf, 5);
        sendData(sock, val, valSize);
    }
};

#endif //TKDATABASE_TK_COMMAND_H
