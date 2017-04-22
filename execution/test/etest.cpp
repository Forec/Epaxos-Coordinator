//
// Created by forec on 17-4-21.
//

#include "etest.h"

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
    // replica 0
    replica_server_param_t * r = (replica_server_param_t *)malloc(sizeof(replica_server_param_t));
    r->group_size = 3;
    r->string_path = (char *)malloc(sizeof(char) * 101);
    REQUIRE( 0 == init_replica_server(r) );
    r->flag |= REPLY_MASK;
    REQUIRE( 0 != (r->flag & REPLY_MASK) );
    Tkdatabase_t * db = new Tkdatabase_t();
    REQUIRE( NULL != db );
    db->Tkdb = unordered_map<string, datanode_t*>();
    init_tkdatabase(db);
    r->statemachine = db;

    uint8_t i, j;
    char path1[5] = "/usr", path2[6] = "/home";
    char vals[3][7] = {"hello", "epaxos", "rdma"};

    tk_command_t * commands = (tk_command_t *)malloc(sizeof(tk_command_t) * 5);
    REQUIRE( NULL != commands );
    for (i = 0; i < 3; ++i) {
        commands[i].opcode = PUT;
        commands[i].key = path1;
        commands[i].val = vals[i];
    }
    commands[3].opcode = GET;
    commands[3].key = path2;
    commands[4].opcode = GET;
    commands[4].key = path1;

    tk_instance_t * instances = (tk_instance_t *)malloc(sizeof(tk_instance_t) * 5);
    for (i = 0; i < 5; ++i) {
        instances[i].cmds = commands + i;
        instances[i].cmds_count = 1;
        instances[i].seq = i + 1;
        instances[i].deps = (unsigned int *)malloc(sizeof(unsigned int) * 3);
        instances[i].flag |= USED_MASK;
        instances[i].flag |= COMMITTED_MASK;
        instances[i].flag &= (~EXECUTED_MASK);
        instances[i].dfn = instances[i].low = 0;
        REQUIRE( (instances[i].flag & EXECUTED_MASK) == 0 );
    }

    r->MaxInstanceNum[0] = 2;
    r->MaxInstanceNum[1] = 2;
    r->MaxInstanceNum[2] = 1;

    for (i = 0; i < 3; ++i) {
        r->executeupto[i] = 0;
        instances[0].deps[i] = 0;
        instances[1].deps[i] = 1;
        instances[2].deps[i] = 1;
        instances[3].deps[i] = 0;
        instances[4].deps[i] = 1;
    }

    r->InstanceMatrix[1][1] = instances[0];
    r->InstanceMatrix[2][1] = instances[1];
    r->InstanceMatrix[0][1] = instances[2];
    r->InstanceMatrix[1][2] = instances[3];
    r->InstanceMatrix[0][2] = instances[4];

    SECTION("tarjan can execute instances in correct order") {
        instance_stack_size = 100;
        instance_stack = (tk_instance_t **) malloc(instance_stack_size * sizeof(tk_instance_t *));
        REQUIRE( NULL != instance_stack );
        REQUIRE( 1 == execute_instance(r, 0, 2) );
        REQUIRE( (r->InstanceMatrix[1][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[2][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[0][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[1][2].flag & EXECUTED_MASK) == 0 );
        REQUIRE( (r->InstanceMatrix[0][2].flag & EXECUTED_MASK) != 0 );
    }

    SECTION("tarjan needs to wait uncommitted instances") {
        instance_stack_size = 100;
        instance_stack = (tk_instance_t **) malloc(instance_stack_size * sizeof(tk_instance_t *));
        r->InstanceMatrix[2][1].flag &= (~COMMITTED_MASK);
        REQUIRE( 0 == (r->InstanceMatrix[2][1].flag & COMMITTED_MASK) );
        REQUIRE( 0 == (r->InstanceMatrix[2][1].flag & EXECUTED_MASK) );

        my_timer_t timeout_watcher;
        struct ev_loop *loop = ev_loop_new(EVFLAG_AUTO);
        timeout_watcher.instance = &(r->InstanceMatrix[2][1]);
        ev_timer_init(&timeout_watcher.timer, commit_timeout_cb, 0.01, 0.);
        ev_timer_start(loop, &timeout_watcher.timer);
        ev_run(loop, 0);

        REQUIRE( 1 == execute_instance(r, 0, 2) );
        REQUIRE( 0 != (r->InstanceMatrix[2][1].flag & EXECUTED_MASK) );
        REQUIRE( (r->InstanceMatrix[1][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[2][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[0][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[1][2].flag & EXECUTED_MASK) == 0 );
        REQUIRE( (r->InstanceMatrix[0][2].flag & EXECUTED_MASK) != 0 );
    }

    SECTION("execution thread should execute all instances") {
        r->flag &= (~SHUTDOWN_MASK);
        REQUIRE( 0 == (r->flag & SHUTDOWN_MASK) );

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

        REQUIRE( (r->InstanceMatrix[1][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[2][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[0][1].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[1][2].flag & EXECUTED_MASK) != 0 );
        REQUIRE( (r->InstanceMatrix[0][2].flag & EXECUTED_MASK) != 0 );
    }
}

static void commit_timeout_cb(EV_P_ ev_timer * w_, int r) {
    my_timer_t * w = (my_timer_t *) w_;
    tk_instance_t * instance = w->instance;
    REQUIRE( 0 == (instance->flag & COMMITTED_MASK) );
    instance->flag |= COMMITTED_MASK;
    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
};

static void shutdown_timeout_cb(EV_P_ ev_timer * w_, int r) {
    my_timer_t * w = (my_timer_t *) w_;
    replica_server_param_t * replica = w->replica;
    REQUIRE( (replica->flag & SHUTDOWN_MASK) == 0 );
    replica->flag |= SHUTDOWN_MASK;
    ev_timer_stop(loop, &(w->timer));
    ev_break(loop, EVBREAK_ALL);
}

void * execute_thread_t(void * arg){
    replica_server_param_t * r = (replica_server_param_t *)arg;
    execute_thread(r);
    return (void*)NULL;
}