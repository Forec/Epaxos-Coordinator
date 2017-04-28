//
// Created by forec on 17-4-27.
//

#include "utest.h"

TEST_CASE("message queue", "mq") {
    MsgQueue_t * mq = initMsgQueue(TEST_SIZE, MSG_SIZE);

    REQUIRE(NULL != mq);
    REQUIRE(0 == availableMsgCount(mq));
    REQUIRE(0 == mq->used);
    REQUIRE(0 == mq->size);
    REQUIRE(TEST_SIZE * MSG_SIZE == mq->cap);
    REQUIRE(TEST_SIZE == mq->cnt);

    Test_Struct_t * test_structs = (Test_Struct_t *) malloc (sizeof(Test_Struct_t) * TEST_SIZE * 2);
    REQUIRE(NULL != test_structs);

    srand((unsigned int)time(NULL));
    int lp;
    for (lp = 0; lp < TEST_SIZE * 2; ++lp) {
        test_structs[lp].id = lp;
        test_structs[lp].count = rand() % 100;
        test_structs[lp].buf_len = 10 + rand() % 25;
        test_structs[lp].buf = (char *) malloc ((size_t)test_structs[lp].buf_len);
        int j;
        for (j = 0; j < test_structs[lp].buf_len - 1; ++ j)
            test_structs[lp].buf[j] = (char)('a' + test_structs[lp].id);
        test_structs[lp].buf[test_structs[lp].buf_len - 1] = '\0';
    }
    
    SECTION("single thread") {
        putIntoMsgQueue(mq, &test_structs);
        REQUIRE(1 == availableMsgCount(mq));
        REQUIRE(0 == mq->used);
        REQUIRE(MSG_SIZE == mq->size);

        int i;
        for (i = 1; i < TEST_SIZE; ++i) {
            Test_Struct_t *p = &test_structs[i];
            putIntoMsgQueue(mq, &p);
        }

        REQUIRE(TEST_SIZE == availableMsgCount(mq));
        REQUIRE(0 == mq->used);
        REQUIRE(TEST_SIZE * MSG_SIZE == mq->size);

        getNextMsg(mq);
        Test_Struct_t *p = *(Test_Struct_t **) getNextMsg(mq);

        REQUIRE(TEST_SIZE - 2 == availableMsgCount(mq));
        REQUIRE(NULL != p);
        REQUIRE(test_structs[1].id == p->id);
        REQUIRE(test_structs[1].count == p->count);
        REQUIRE(test_structs[1].buf_len == p->buf_len);
        REQUIRE(!strcmp(test_structs[1].buf, p->buf));
    }

    SECTION("multiple threads") {
        mq_timer_t timeout_watcher;
        timeout_watcher.p = test_structs;
        timeout_watcher.mq = mq;

        param_t *ptr = (param_t *) malloc(sizeof(param_t));
        ptr->mq = mq;
        ptr->test_structs = test_structs;

        struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);
        ev_timer_init(&timeout_watcher.timer, readMsgQueue_cb, 0.1, 0.);
        ev_timer_start(loop, &timeout_watcher.timer);

        pthread_t thread;
        int pid = pthread_create(&thread, NULL, putMsgQueue_thread, (void *) ptr);
        REQUIRE(pid >= 0);

        ev_run(loop, 0);
        pthread_join(thread, NULL);

        // callback has read one msg from mq, so there are still 6 left (1..6)
        REQUIRE(TEST_SIZE == availableMsgCount(mq));

        int i;
        for (i = 0; i < TEST_SIZE - 1; ++i) {
            getNextMsg(mq);
        }
        Test_Struct_t *p = *(Test_Struct_t **) getNextMsg(mq);
        REQUIRE(NULL != p);
        REQUIRE(TEST_SIZE == p->id);
        REQUIRE(&test_structs[TEST_SIZE] == p);
    }


    SECTION("race condition on multiple reads") {
        param_t *ptr = (param_t *) malloc(sizeof(param_t));
        ptr->mq = mq;
        ptr->test_structs = test_structs;

        int i, pid[TEST_SIZE];
        pthread_t thread[TEST_SIZE];
        for (i = 0; i < TEST_SIZE; ++i) {
            pid[i] = pthread_create(&thread[i], NULL, readMsgQueue_thread, (void *) ptr);
            REQUIRE(pid[i] >= 0);
        }

        for (i = 0; i < TEST_SIZE * 2; ++i) {
            Test_Struct_t **p = (Test_Struct_t **)malloc(sizeof(Test_Struct_t*));
            *p = &test_structs[i];
            putIntoMsgQueue(mq, p);
        }

        for (i = 0; i < TEST_SIZE; ++i)
            pthread_join(thread[i], NULL);

        free(ptr);
        free(test_structs);
        destroyMsgQueue(mq);

        REQUIRE(0 == availableMsgCount(mq));
    }


    SECTION("race condition on multiple writes") {
        param_t *ptr = (param_t *) malloc(sizeof(param_t));
        ptr->mq = mq;
        ptr->test_structs = test_structs;

        pthread_t thread[TEST_SIZE];
        int i, pid[TEST_SIZE];
        for (i = 0; i < TEST_SIZE; ++i) {
            pid[i] = pthread_create(&thread[i], NULL, putMsgQueue_thread, (void *) ptr);
            REQUIRE(pid[i] >= 0);
        }

        for (i = 0; i < (TEST_SIZE + 1) * TEST_SIZE; ++i) {
            Test_Struct_t *p = *(Test_Struct_t **) getNextMsg(mq);
            char *tmp = (char *) malloc(sizeof(char) * p->buf_len);
            int j;
            for (j = 0; j < p->buf_len - 1; ++j)
                tmp[j] = (char)('a' + p->id);
            tmp[p->buf_len - 1] = '\0';
            REQUIRE(NULL != p->buf);
            REQUIRE(!strcmp(tmp, p->buf));
        }

        for (i = 0; i < TEST_SIZE; ++i)
            pthread_join(thread[i], NULL);

        REQUIRE(0 == availableMsgCount(mq));
    }


    SECTION("race condition on read and write") {

        param_t * ptr = (param_t *) malloc (sizeof(param_t));
        ptr->mq = mq;
        ptr->test_structs = test_structs;

        int i;
        pthread_t thread_write[TEST_SIZE];
        int pid_write[TEST_SIZE];
        for (i = 0; i < TEST_SIZE; ++i) {
            pid_write[i] = pthread_create(&thread_write[i], NULL, putMsgQueue_thread, (void *) ptr);
            REQUIRE(pid_write[i] >= 0);
        }

        pthread_t thread_read[TEST_SIZE * (TEST_SIZE + 1) / 2];
        int pid_read[TEST_SIZE * (TEST_SIZE + 1) / 2];
        for (i = 0; i < TEST_SIZE * (TEST_SIZE + 1) / 2; ++i) {
            pid_read[i] = pthread_create(&thread_read[i], NULL, readMsgQueue_thread, (void *) ptr);
            REQUIRE(pid_read[i] >= 0);
        }

        for (i = 0; i < TEST_SIZE; ++i)
            pthread_join(thread_write[i], NULL);
        for (i = 0; i < TEST_SIZE * (TEST_SIZE + 1) / 2; ++i)
            pthread_join(thread_read[i], NULL);

        REQUIRE(0 == availableMsgCount(mq));
    }
}

static void readMsgQueue_cb(EV_P_ ev_timer * w_, int r) {
    mq_timer_t * w = (mq_timer_t *) w_;
    MsgQueue_t * mq = w->mq;
    REQUIRE(NULL != mq);

    // mq should be blocked when full
    REQUIRE(TEST_SIZE == availableMsgCount(mq));

    Test_Struct_t * p = *(Test_Struct_t **) getNextMsg(mq);
    REQUIRE(NULL != p);
    REQUIRE(0 == p->id);

    int i;
    char * tmp = (char *)malloc(sizeof(char) * p->buf_len);
    for (i = 0; i < p->buf_len - 1; ++i)
        tmp[i] = (char)('a' + p->id);
    tmp[p->buf_len - 1] = '\0';
    REQUIRE(NULL != p->buf);
    REQUIRE(!strcmp(tmp, p->buf));

    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
}

void * readMsgQueue_thread(void * arg) {
    param_t * param = (param_t *)arg;
    REQUIRE(NULL != param);
    MsgQueue_t * mq = param->mq;
    REQUIRE(NULL != mq);
    Test_Struct_t * src = param->test_structs;
    REQUIRE(NULL != src);
    Test_Struct_t * p = *(Test_Struct_t **) getNextMsg(mq);
    REQUIRE(NULL != p);
    REQUIRE(0 <= p->id);
    REQUIRE(TEST_SIZE * 2 > p->id);
    REQUIRE(&src[p->id] == p);
    getNextMsg(mq);
    return (void*)NULL;
}

void * putMsgQueue_thread(void * arg) {
    param_t * param = (param_t *)arg;
    REQUIRE(NULL != param);
    MsgQueue_t * mq = param->mq;
    REQUIRE(NULL != mq);
    Test_Struct_t * src = param->test_structs;
    REQUIRE(NULL != src);
    int i;
    for (i = 0; i <= TEST_SIZE; ++i) {
        Test_Struct_t ** p = (Test_Struct_t **)malloc(sizeof(Test_Struct_t *));
        *p = &src[i];
        putIntoMsgQueue(mq, p);
    }
    return (void*)NULL;
}