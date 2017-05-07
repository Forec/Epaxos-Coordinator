//
// Created by forec on 17-5-7.
//

#include "../include/communication.h"

int listenOn(uint16_t port) {
    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    int _socket = socket(PF_INET, SOCK_STREAM, 0);
    if (_socket < 0)
        return _socket;
    if (bind(_socket, (struct sockaddr *)&addr, sizeof(addr)))
        return -1;
    if (listen(_socket, GROUP_SZIE))
        return -1;
    return _socket;
}

int acceptAt(int sock) {
    struct sockaddr_in remoteAddr;
    socklen_t length = sizeof(remoteAddr);
    int nsock = accept(sock, (struct sockaddr *)&remoteAddr, &length);
    return nsock;
}

int dialTo(const std::string & remoteIP, uint16_t port) {
    struct sockaddr_in localAddr;
    bzero(&localAddr, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htons(INADDR_ANY);
    localAddr.sin_port = htons(0);
    int localSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (localSocket < 0)
        return localSocket;
    if (bind(localSocket, (struct sockaddr*) &localAddr, sizeof(localAddr)))
        return -1;
    struct sockaddr_in remoteAddr;
    bzero(&remoteAddr, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    if (inet_aton(remoteIP.c_str(), &remoteAddr.sin_addr) == 0)
        return -1;
    remoteAddr.sin_port = htons(port);
    socklen_t remoteAddrLength = sizeof(remoteAddr);
    if (connect(localSocket, (struct sockaddr*) & remoteAddr, remoteAddrLength) < 0)
        return -1;
    return localSocket;
}

long sendData(int sock, char * buf, size_t len) {
    ssize_t sent = 0;
    while (len > 0) {
        ssize_t realWriteCount = write(sock, buf + sent, len);
        if (realWriteCount < 0) {
            fprintf(stderr, "Connection error! Cannot send to sock %d!\n", sock);
            return realWriteCount;
        }
        len -= realWriteCount;
        sent += realWriteCount;
    }
    return sent;
}

ssize_t readUntil(int sock, char * buf, size_t len) {
    ssize_t readed = (ssize_t )len;
    while (readed > 0) {
        ssize_t cur = read(sock, buf, (size_t)readed);
        if (cur < 0) {
            fprintf(stderr, "Connection error! Cannot read from sock %d!\n", sock);
            return cur;
        }
        readed -= cur;
    }
    return readed;
}

void destroyConnection(int sock) {
    close(sock);
}
