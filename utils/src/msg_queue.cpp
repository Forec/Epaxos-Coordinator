//
// Created by forec on 17-4-27.
//

#include "../include/msg_queue.h"

MsgQueue::MsgQueue(uint64_t _cap, uint64_t _msgSize) {
    used = 0;
    cap = _cap * _msgSize;
    cnt = _cap;
    size = 0;
    msgSize = _msgSize;
    buf = new char[cap];
    empty = new semaphore(cnt);
    full = new semaphore(0);
}

MsgQueue::~MsgQueue() {
    if (empty)
        delete empty;
    if (full)
        delete full;
    if (buf)
        delete [] buf;
}

uint64_t MsgQueue::count() {
    return full->getCount();
}

bool MsgQueue::hasNext() {
    return count() > 0;
}

void* MsgQueue::get() {
    void * p;
    full->wait();
    mutex.lock();
    p = (void *)(buf + used);
    used += msgSize;
    if (used >= cap) {
        used = 0;
    }
    mutex.unlock();
    empty->notify();
    return p;
}

void MsgQueue::put(void * src) {
    empty->wait();
    mutex.lock();
    if (size == cap) {
        size = 0;
    }
    uint64_t pos = size;
    size += msgSize;
    mutex.unlock();
    memcpy(buf + pos, src, msgSize);
    full->notify();
}
