//
// Created by forec on 17-5-6.
//

#ifndef TKDATABASE_COMMUNICATION_H
#define TKDATABASE_COMMUNICATION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <libnet.h>
#include "../../config/config.h"

int listenOn(uint16_t port);

int acceptAt(int sock);

int dialTo(const std::string & remoteIP, uint16_t port);

long sendData(int sock, char * buf, size_t len);

ssize_t readUntil(int sock, char * buf, size_t len);

void destroyConnection(int sock);

#endif //TKDATABASE_COMMUNICATION_H
