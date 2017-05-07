//
// Created by forec on 17-4-27.
//

#include "utest.h"

std::atomic<uint32_t> passed(0);
std::atomic<uint32_t> failed(0);

// global used functions
void init();

// Message Queue Test Cases
void test_single_thread(MsgQueue * mq, Test_Struct * test_structs);
void test_multiple_thread(MsgQueue * mq, Test_Struct * test_structs);
void test_read_race(MsgQueue * mq, Test_Struct * test_structs);
void test_write_race(MsgQueue * mq, Test_Struct * test_structs);
void test_read_write_race(MsgQueue * mq, Test_Struct * test_structs);
void test_msg_queue();
void clear_msg_queue();

// Socket Module Test Cases
void test_socket_module();

int main() {
    // msg queue test
    test_msg_queue();

    // socket module test
    test_socket_module();

    return 0;
}


/*****************************************************************************************
 *                                Message Queue Test                                     *
 ****************************************************************************************/

void clear_msg_queue(MsgQueue * mq) {
    while (mq->hasNext())
        mq->get();
}


void test_single_thread(MsgQueue * mq, Test_Struct * test_structs){
    mq->put(&test_structs);
    alert( 1 == mq->count() );

    for (int i = 1; i < TEST_SIZE; ++i) {
        Test_Struct *p = &test_structs[i];
        mq->put(&p);
    }

    alert( TEST_SIZE == mq->count() );

    mq->get();
    Test_Struct *p = *(Test_Struct **) mq->get();

    alert( TEST_SIZE - 2 == mq->count() );
    alert( NULL != p );
    alert( test_structs[1].id == p->id );
    alert( test_structs[1].count == p->count );
    alert( test_structs[1].buf_len == p->buf_len );
    alert( !strcmp(test_structs[1].buf, p->buf) );

    clear_msg_queue(mq);
}

void test_multiple_thread(MsgQueue * mq, Test_Struct * test_structs) {
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
    alert( TEST_SIZE == mq->count() );

    for (int i = 0; i < TEST_SIZE - 1; ++i) {
        mq->get();
    }
    Test_Struct * p = *(Test_Struct **) mq->get();
    alert( NULL != p );
    alert( TEST_SIZE == p->id );
    alert( &test_structs[TEST_SIZE] == p );

    clear_msg_queue(mq);
}

void test_read_race(MsgQueue * mq, Test_Struct * test_structs) {
    std::array<std::thread *, TEST_SIZE> pts;
    for (int i = 0; i < TEST_SIZE; ++i) {
        pts[i] = new std::thread(readMsgQueue_thread, mq, test_structs);
    }

    for (int i = 0; i < TEST_SIZE * 2; ++i) {
        Test_Struct *p = &test_structs[i];
        mq->put(&p);
    }

    for (int i = 0; i < pts.size(); i++) {
        pts[i]->join();
        delete pts[i];
    }

    alert( 0 == mq->count() );

    clear_msg_queue(mq);
}

void test_write_race(MsgQueue * mq, Test_Struct * test_structs) {
    std::array<std::thread *, TEST_SIZE> pts;
    for (int i = 0; i < TEST_SIZE; ++i) {
        pts[i] = new std::thread(putMsgQueue_thread, mq, test_structs);
    }

    for (int i = 0; i < (TEST_SIZE + 1) * TEST_SIZE; ++i) {
        Test_Struct * p = *(Test_Struct **) mq->get();
        alert( p != NULL );
        char *tmp = new char[p->buf_len];
        for (int j = 0; j < p->buf_len - 1; ++j)
            tmp[j] = (char)('a' + p->id);
        tmp[p->buf_len - 1] = '\0';
        alert( NULL != p->buf );
        alert( !strcmp(tmp, p->buf) );
        delete [] tmp;
    }

    for (int i = 0; i < pts.size(); i++) {
        pts[i]->join();
        delete pts[i];
    }

    alert( 0 == mq->count() );
    clear_msg_queue(mq);
}

void test_read_write_race(MsgQueue * mq, Test_Struct * test_structs) {
    std::array<std::thread *, TEST_SIZE> ptws;
    for (int i = 0; i < TEST_SIZE; ++i) {
        ptws[i] = new std::thread(putMsgQueue_thread, mq, test_structs);
    }

    std::array<std::thread *, TEST_SIZE * (TEST_SIZE + 1) / 2> ptrs;
    for (int i = 0; i < TEST_SIZE * (TEST_SIZE + 1) / 2; ++i) {
        ptrs[i] = new std::thread(readMsgQueue_thread, mq, test_structs);
    }

    for (int i = 0; i < ptws.size(); i++) {
        ptws[i]->join();
        delete ptws[i];
    }

    for (int i = 0; i < ptrs.size(); i++) {
        ptrs[i]->join();
        delete ptrs[i];
    }

    alert( 0 == mq->count() );
    clear_msg_queue(mq);
}


void test_msg_queue() {
    init();
    MsgQueue * mq = new MsgQueue(TEST_SIZE, MSG_SIZE);

    alert(NULL != mq);
    alert( 0 == mq->count() );

    Test_Struct * test_structs = new Test_Struct[TEST_SIZE * 2];
    alert( NULL != test_structs );

    srand((unsigned int)time(NULL));

    for (int lp = 0; lp < TEST_SIZE * 2; ++lp) {
        test_structs[lp].id = lp;
        test_structs[lp].count = rand() % 100;
        test_structs[lp].buf_len = 10 + rand() % 25;
        test_structs[lp].buf = new char[test_structs[lp].buf_len];
        int j;
        for (j = 0; j < test_structs[lp].buf_len - 1; ++ j)
            test_structs[lp].buf[j] = (char)('a' + test_structs[lp].id);
        test_structs[lp].buf[test_structs[lp].buf_len - 1] = '\0';
    }

    test_single_thread(mq, test_structs);
    test_multiple_thread(mq, test_structs);
    test_read_race(mq, test_structs);
    test_write_race(mq, test_structs);
    test_read_write_race(mq, test_structs);

    printf("=================== TEST MESSAGE QUEUE ===================\n");
    printf("                passed: %d, failed: %d\n", passed.load(), failed.load());
    delete mq;
    delete [] test_structs;
}

static void readMsgQueue_cb(EV_P_ ev_timer * w_, int r) {
    mq_timer * w = (mq_timer *) w_;
    MsgQueue * mq = w->mq;
    alert( NULL != mq );

    // mq should be blocked when full
    alert( TEST_SIZE == mq->count() );

    Test_Struct * p = *(Test_Struct **) mq->get();
    alert( NULL != p );
    alert( 0 == p->id );

    char * tmp = new char[p->buf_len];
    for (int i = 0; i < p->buf_len - 1; ++i)
        tmp[i] = (char)('a' + p->id);
    tmp[p->buf_len - 1] = '\0';
    alert( NULL != p->buf );
    alert( !strcmp(tmp, p->buf) );

    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
    delete [] tmp;
}

void readMsgQueue_thread(MsgQueue *mq, Test_Struct *src) {
    alert(NULL != mq);
    alert(NULL != src);
    Test_Struct *p = *(Test_Struct **) mq->get();
    alert(NULL != p);
    alert(0 <= p->id);
    alert(TEST_SIZE * 2 > p->id);
    alert(&src[p->id] == p);
    mq->get();
}

void putMsgQueue_thread(MsgQueue * mq, Test_Struct * src) {
    alert(NULL != mq);
    alert(NULL != src);
    for (int i = 0; i <= TEST_SIZE; ++i) {
        Test_Struct * p = &src[i];
        mq->put(&p);
    }
}

/***************************************************************************************
 *                                  Socket Module Test                                 *
 **************************************************************************************/


void test_socket_module() {
    init();
    int sock = listenOn(9001);
    alert( sock >= 0 );
    int connected = 0;
    std::thread t1(communication_thread, 0);
    std::thread t2(communication_thread, 1);
    std::vector<std::thread *> pts;
    while (connected < 2) {
        int nsock = acceptAt(sock);
        alert( nsock >= 0 );
        pts.push_back(new std::thread(listen_thread, nsock));
        connected++;
    }
    t1.join();
    t2.join();
    for (int i = 0; i < pts.size(); i++) {
        pts[i]->join();
        delete pts[i];
    }
    alert( connected == 2 );
    destroyConnection(sock);
    printf("=================== TEST SOCKET MODULE ===================\n");
    printf("                passed: %d, failed: %d\n", passed.load(), failed.load());
}

void communication_thread(int idx) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    int sock = dialTo("127.0.0.1", 9001);
    alert( sock >= 0 );
    int64_t sendNum = idx == 0 ? 0xAAAAAAAAAAAAAAAA : 0x0101010101010101;
    alert( sendData(sock, (char *)&sendNum, 8) == 8 );
    destroyConnection(sock);
}

void listen_thread(int sock) {
    alert( sock >= 0 );
    int64_t received;
    alert( 0 == readUntil(sock, (char *)&received, 8) );
    bool isValid = received == 0xAAAAAAAAAAAAAAAA || received == 0x0101010101010101;
    alert( isValid );
    destroyConnection(sock);
}

/***************************************************************************************
 *                                  GLOBAL USED FUNCTIONS                              *
 **************************************************************************************/

void init() {
    passed.store(0);
    failed.store(0);
}