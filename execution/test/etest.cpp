//
// Created by forec on 17-4-21.
//

#include "etest.h"

using namespace std;

/*
 * 1: put /usr "hello"      // replica 1
 * 1: put /usr "expaos"     // replica 2
 * 1: put /usr "rdma"       // replica 0
 * 2: get /home             // replica 1
 * 2: get /usr              // replica 0
 *
 * dependency cycle: (replica Id, instance Index)
 *    ((1, 1), (2, 1), (0, 1)) <-- (0, 2)  (1, 2)
 */

TEST_CASE("Instance execution", "exec") {

    printf("fuck ehre\n");
    fflush(stdout);
    // replica 0
    Replica * r = new Replica();
    r->group_size = 3;
    r->path = "";
    REQUIRE( r->init() );
    r->Dreply = true;
    Tkdatabase_t * db = new Tkdatabase_t();
    REQUIRE(nullptr != db );
    db->Tkdb = unordered_map<string, datanode_t*>();
    init_tkdatabase(db);
    r->statemachine = db;

    string path1 = "/usr", path2 = "/home";
    char vals[3][8] = {"hello", "epaxos", "rdma"};

    tk_command * commands = new tk_command[5];
    REQUIRE( nullptr != commands );
    for (int i = 0; i < 3; ++i) {
        commands[i].opcode = PUT;
        commands[i].key = path1;
        commands[i].val = vals[i];
    }
    commands[3].opcode = GET;
    commands[3].key = path2;
    commands[4].opcode = GET;
    commands[4].key = path1;

    tk_instance * instances = new tk_instance[5];
    for (int i = 0; i < 5; ++i) {
        instances[i].cmds.clear();
        instances[i].cmds.push_back(commands[i]);
        instances[i].seq = i + 1;
        instances[i].status = COMMITTED;
        instances[i].dfn = instances[i].low = 0;
        REQUIRE( instances[i].status == COMMITTED );
    }

    r->crtInstance[0] = 2;
    r->crtInstance[1] = 2;
    r->crtInstance[2] = 1;

    for (int i = 0; i < 3; ++i) {
        r->executeUpTo[i] = 0;
        instances[0].deps[i] = 0;
        instances[1].deps[i] = 1;
        instances[2].deps[i] = 1;
        instances[3].deps[i] = 0;
        instances[4].deps[i] = 1;
    }

    r->InstanceMatrix[1][1] = &instances[0];
    r->InstanceMatrix[2][1] = &instances[1];
    r->InstanceMatrix[0][1] = &instances[2];
    r->InstanceMatrix[1][2] = &instances[3];
    r->InstanceMatrix[0][2] = &instances[4];

    SECTION("tarjan can execute instances in correct order") {
        instance_stack_size = 100;
        instance_stack = (tk_instance **) malloc(instance_stack_size * sizeof(tk_instance *));
        REQUIRE(nullptr != instance_stack );
        REQUIRE( execute_instance(r, 0, 2) );
        REQUIRE( r->InstanceMatrix[1][1]->status == EXECUTED  );
        REQUIRE( r->InstanceMatrix[2][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[0][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[1][2]->status != EXECUTED );
        REQUIRE( r->InstanceMatrix[0][2]->status == EXECUTED );
    }

    SECTION("tarjan needs to wait uncommitted instances") {
        instance_stack_size = 100;
        instance_stack = (tk_instance **) malloc(instance_stack_size * sizeof(tk_instance *));
        r->InstanceMatrix[2][1]->status = NONE;
        REQUIRE( r->InstanceMatrix[2][1]->status != COMMITTED );
        REQUIRE( r->InstanceMatrix[2][1]->status != EXECUTED );

        my_timer_t timeout_watcher;
        struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);
        timeout_watcher.instance = r->InstanceMatrix[2][1];
        ev_timer_init(&timeout_watcher.timer, commit_timeout_cb, 0.01, 0.);
        ev_timer_start(loop, &timeout_watcher.timer);
        ev_run(loop, 0);

        REQUIRE( 1 == execute_instance(r, 0, 2) );
        REQUIRE( r->InstanceMatrix[2][1]->status != EXECUTED );
        REQUIRE( r->InstanceMatrix[1][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[0][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[1][2]->status != EXECUTED );
        REQUIRE( r->InstanceMatrix[0][2]->status == EXECUTED );
    }

    SECTION("execution thread should execute all instances") {
        r->Shutdown = false;
        REQUIRE( !r->Shutdown );

        my_timer_t timeout_watcher;
        struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);
        timeout_watcher.replica = r;
        ev_timer_init(&timeout_watcher.timer, shutdown_timeout_cb, 0.1, 0.);
        ev_timer_start(loop, &timeout_watcher.timer);

        pthread_t thread;
        int pid = pthread_create(&thread, NULL, execute_thread_t, (void *)r);
        REQUIRE( pid >= 0 );

        ev_run(loop, 0);
        pthread_join(thread, NULL);

        REQUIRE( r->InstanceMatrix[1][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[2][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[0][1]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[1][2]->status == EXECUTED );
        REQUIRE( r->InstanceMatrix[0][2]->status == EXECUTED );
    }
}

static void commit_timeout_cb(EV_P_ ev_timer * w_, int r) {
    my_timer_t * w = (my_timer_t *) w_;
    tk_instance * instance = w->instance;
    REQUIRE( instance->status != COMMITTED );
    instance->status = COMMITTED;
    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
};

static void shutdown_timeout_cb(EV_P_ ev_timer * w_, int r) {
    my_timer_t * w = (my_timer_t *) w_;
    Replica * replica = w->replica;
    REQUIRE( !replica->Shutdown );
    replica->Shutdown = true;
    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
}

void * execute_thread_t(void * arg){
    Replica * r = (Replica *)arg;
    execute_thread(r);
    return (void*)NULL;
}