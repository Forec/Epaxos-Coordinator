//
// Created by forec on 17-4-27.
//

#include "utest.h"

TEST_CASE("message queue", "mq") {
    MsgQueue * mq = new MsgQueue(TEST_SIZE, MSG_SIZE);

    REQUIRE(NULL != mq);
    REQUIRE( 0 == mq->count() );

    Test_Struct * test_structs = new Test_Struct[TEST_SIZE * 2];
    REQUIRE( NULL != test_structs );

    srand((unsigned int)time(NULL));

    for (int lp = 0; lp < TEST_SIZE * 2; ++lp) {
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
        mq->put(&test_structs);
        REQUIRE( 1 == mq->count() );

        for (int i = 1; i < TEST_SIZE; ++i) {
            Test_Struct *p = &test_structs[i];
            mq->put(&p);
        }

        REQUIRE( TEST_SIZE == mq->count() );

        mq->get();
        Test_Struct *p = *(Test_Struct **) mq->get();

        REQUIRE( TEST_SIZE - 2 == mq->count() );
        REQUIRE( NULL != p );
        REQUIRE( test_structs[1].id == p->id );
        REQUIRE( test_structs[1].count == p->count );
        REQUIRE( test_structs[1].buf_len == p->buf_len );
        REQUIRE( !strcmp(test_structs[1].buf, p->buf) );
    }

    SECTION("multiple threads") {
        mq_timer timeout_watcher;
        timeout_watcher.p = test_structs;
        timeout_watcher.mq = mq;

        struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);
        ev_timer_init(&timeout_watcher.timer, readMsgQueue_cb, 0.1, 0.);
        ev_timer_start(loop, &timeout_watcher.timer);

        std::thread p1(putMsgQueue_thread, mq, test_structs);
        ev_run(loop, 0);
        p1.join();

        // callback has read one msg from mq, so there are still 6 left (1..6)
        REQUIRE( TEST_SIZE == mq->count() );

        for (int i = 0; i < TEST_SIZE - 1; ++i) {
            mq->get();
        }
        Test_Struct * p = *(Test_Struct **) mq->get();
        REQUIRE( NULL != p );
        REQUIRE( TEST_SIZE == p->id );
        REQUIRE( &test_structs[TEST_SIZE] == p );
    }


    SECTION("race condition on multiple reads") {
        std::array<std::thread *, TEST_SIZE> pts;
        for (int i = 0; i < TEST_SIZE; ++i) {
            pts[i] = new std::thread(readMsgQueue_thread, mq, test_structs);
        }

        for (int i = 0; i < TEST_SIZE * 2; ++i) {
            Test_Struct *p = &test_structs[i];
            mq->put(&p);
        }

        for (int i = 0; i < pts.size(); i++)
            pts[i]->join();

        REQUIRE( 0 == mq->count() );
//        delete mq;
//        delete [] test_structs;
    }


//    SECTION("race condition on multiple writes") {
//        param_t *ptr = (param_t *) malloc(sizeof(param_t));
//        ptr->mq = mq;
//        ptr->test_structs = test_structs;
//
//        pthread_t thread[TEST_SIZE];
//        int i, pid[TEST_SIZE];
//        for (i = 0; i < TEST_SIZE; ++i) {
//            pid[i] = pthread_create(&thread[i], NULL, putMsgQueue_thread, (void *) ptr);
//            REQUIRE(pid[i] >= 0);
//        }
//
//        for (i = 0; i < (TEST_SIZE + 1) * TEST_SIZE; ++i) {
//            Test_Struct_t *p = *(Test_Struct_t **) getNextMsg(mq);
//            char *tmp = (char *) malloc(sizeof(char) * p->buf_len);
//            int j;
//            for (j = 0; j < p->buf_len - 1; ++j)
//                tmp[j] = (char)('a' + p->id);
//            tmp[p->buf_len - 1] = '\0';
//            REQUIRE(NULL != p->buf);
//            REQUIRE(!strcmp(tmp, p->buf));
//        }
//
//        for (i = 0; i < TEST_SIZE; ++i)
//            pthread_join(thread[i], NULL);
//
//        REQUIRE(0 == availableMsgCount(mq));
//    }
//
//
//    SECTION("race condition on read and write") {
//
//        param_t * ptr = (param_t *) malloc (sizeof(param_t));
//        ptr->mq = mq;
//        ptr->test_structs = test_structs;
//
//        int i;
//        pthread_t thread_write[TEST_SIZE];
//        int pid_write[TEST_SIZE];
//        for (i = 0; i < TEST_SIZE; ++i) {
//            pid_write[i] = pthread_create(&thread_write[i], NULL, putMsgQueue_thread, (void *) ptr);
//            REQUIRE(pid_write[i] >= 0);
//        }
//
//        pthread_t thread_read[TEST_SIZE * (TEST_SIZE + 1) / 2];
//        int pid_read[TEST_SIZE * (TEST_SIZE + 1) / 2];
//        for (i = 0; i < TEST_SIZE * (TEST_SIZE + 1) / 2; ++i) {
//            pid_read[i] = pthread_create(&thread_read[i], NULL, readMsgQueue_thread, (void *) ptr);
//            REQUIRE(pid_read[i] >= 0);
//        }
//
//        for (i = 0; i < TEST_SIZE; ++i)
//            pthread_join(thread_write[i], NULL);
//        for (i = 0; i < TEST_SIZE * (TEST_SIZE + 1) / 2; ++i)
//            pthread_join(thread_read[i], NULL);
//
//        REQUIRE(0 == availableMsgCount(mq));
//    }
}

//TEST_CASE("socket communication module", "sock") {
//    int sock = listenOn(9001);
//    REQUIRE( sock >= 0 );
//    int connected = 0;
//    int param = 1;
//    go(communication_thread, &param);
//    param = 2;
//    go(communication_thread, &param);
//    while (connected < 2) {
//        int nsock = acceptAt(sock);
//        REQUIRE( nsock >= 0 );
//        go(listen_thread, &nsock);
//        connected++;
//    }
//    REQUIRE( connected == 2 );
//    destroyConnection(sock);
//}

static void readMsgQueue_cb(EV_P_ ev_timer * w_, int r) {
    mq_timer * w = (mq_timer *) w_;
    MsgQueue * mq = w->mq;
    REQUIRE( NULL != mq );

    // mq should be blocked when full
    REQUIRE( TEST_SIZE == mq->count() );

    Test_Struct * p = *(Test_Struct **) mq->get();
    REQUIRE( NULL != p );
    REQUIRE( 0 == p->id );

    char * tmp = new char[p->buf_len];
    for (int i = 0; i < p->buf_len - 1; ++i)
        tmp[i] = (char)('a' + p->id);
    tmp[p->buf_len - 1] = '\0';
    REQUIRE( NULL != p->buf );
    REQUIRE( !strcmp(tmp, p->buf) );

    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
}

void readMsgQueue_thread(MsgQueue * mq, Test_Struct * src) {
    REQUIRE( NULL != mq );
    REQUIRE( NULL != src );
    Test_Struct * p = *(Test_Struct **) mq->get();
    REQUIRE( NULL != p );
    REQUIRE( 0 <= p->id );
    REQUIRE( TEST_SIZE * 2 > p->id );
    REQUIRE( &src[p->id] == p );
    mq->get();
}

void putMsgQueue_thread(MsgQueue * mq, Test_Struct * src) {
    REQUIRE(NULL != mq);
    REQUIRE(NULL != src);
    for (int i = 0; i <= TEST_SIZE; ++i) {
        Test_Struct * p = &src[i];
        mq->put(&p);
    }
}

//void * communication_thread(void * arg) {
//    int idx = *(int *) arg;
//    nano_sleep(1000 * 1000 * 1000);
//    int sock = dialTo("127.0.0.1", 9001);
//    REQUIRE( sock >= 0 );
//    std::string sendMsg = "hello" + std::to_string(idx);
//    char buf[11];
//    strcpy(buf, sendMsg.c_str());
//    REQUIRE( sendData(sock, buf, sendMsg.size()) == sendMsg.size() );
//    destroyConnection(sock);
//    return (void *)NULL;
//}
//
//void * listen_thread(void * arg) {
//    int sock = *(int *) arg;
//    REQUIRE( sock >= 0 );
//    char buf[11] = {0};
//    REQUIRE( 0 == readUntil(sock, buf, 6) );
//    bool isValid = !strcmp(buf, "hello1") || !strcmp(buf, "hello2");
//    REQUIRE( isValid );
//    destroyConnection(sock);
//}