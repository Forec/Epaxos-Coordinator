//
// Created by forec on 17-5-1.
//

#ifndef TKDATABASE_GO_THREAD_H
#define TKDATABASE_GO_THREAD_H

#include <pthread.h>

int go(void *(*func) (void *), void * param);

#endif //TKDATABASE_GO_THREAD_H
