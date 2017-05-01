//
// Created by forec on 17-5-1.
//

#include "../include/go_thread.h"

int go(void *(*func) (void *), void * param) {
    pthread_t thread;
    return pthread_create(&thread, NULL, func, param);
}
