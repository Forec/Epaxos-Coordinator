//
// Created by forec on 17-4-27.
//

#ifndef __MSG_QUEUE_H_
#define __MSG_QUEUE_H_

#include <thread>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <queue>
#include <atomic>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

class semaphore {
private:
    std::mutex mutex_;
    std::condition_variable condition_;
    uint64_t count_;

public:
    semaphore(uint64_t _c = 0) : count_(_c) {};
    void notify() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        ++count_;
        condition_.notify_one();
    }

    void wait() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        while(!count_)
            condition_.wait(lock);
        --count_;
    }

    bool try_wait() {
        std::unique_lock<decltype(mutex_)> lock(mutex_);
        if(count_) {
            --count_;
            return true;
        }
        return false;
    }
    uint64_t getCount() {
        return count_;
    }
};

class MsgQueue {
private:
    std::atomic<uint64_t> used;
    std::atomic<uint64_t> size;
    uint64_t cap;
    uint64_t cnt;
    uint64_t msgSize;
    std::mutex mutex;
    semaphore * empty;
    semaphore * full;
    char * buf;
public:
    MsgQueue(uint64_t _cap, uint64_t _msgSize);
    ~MsgQueue();
    uint64_t count();
    bool hasNext();
    void* get();
    /*
     * PreAccept * p = *(PreAccept **)mq->get();
     */

    void put(void * src);
    /*
     * PreAccept * p;  -- pointer of a PreAccept msg (received from rdma) to be put into msgQueue
     * mq->put(&p);
     */

};


#endif