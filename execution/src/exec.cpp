//
// Created by forec on 17-4-20.
//

#include "../include/exec.h"

//#define DEBUG_EXEC

unsigned int instance_stack_size;
tk_instance_t ** instance_stack;
unsigned int top;

void nano_sleep(unsigned int ns){
    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = ns;
    nanosleep(&ts, 0);
}

int cmp_instance(const void *p1, const void *p2) {
    if ((*(tk_instance_t **)p1)->seq <  (*(tk_instance_t **)p2)->seq) return -1;
    if ((*(tk_instance_t **)p1)->seq == (*(tk_instance_t **)p2)->seq) return 0;
    return 1;
}

uint8_t
strong_connect(replica_server_param_t * r,
               tk_instance_t * v,
               unsigned short * index)
{
    v->dfn = *index;
    v->low = *index;
    (*index)++;

    if (instance_stack_size == top){
        if (!realloc(STACK, instance_stack_size << 1)){
            info(stderr, "cannot realloc space for instance stack in tarjan.\n");
            return 0;
        };
        instance_stack_size <<= 1;
    }
    unsigned int pos = top;
    STACK[top++] = v;

    unsigned int q;
    uint64_t i, inst;
    for (q = 0; q < r->group_size; ++q) {
        inst = v->deps[q];
        for (i = r->executeupto[q] + 1; i <= inst; ++i) {
            while (!(r->InstanceMatrix[q][i].flag & USED_MASK) ||
                   !r->InstanceMatrix[q][i].cmds ||
                   !v->cmds) {
                nano_sleep(1000 * 1000);
            }
            if (r->InstanceMatrix[q][i].flag & EXECUTED_MASK)
                continue;
            while (!(r->InstanceMatrix[q][i].flag & COMMITTED_MASK)) {
                nano_sleep(1000 * 1000);
            }
            tk_instance_t * w = &(r->InstanceMatrix[q][i]);

            if (!w->dfn) {
                if (!strong_connect(r, w, index)) {
                    int j;
                    for (j = pos; j < top; ++j)
                        STACK[j]->dfn = 0;
                    top = pos;
                    return 0;
                }
                if (w->low < v->low)
                    v->low = w->low;
            } else if (w->dfn < v->low) {
                v->low = w->dfn;
            }
        }
    }

    tk_instance_t * w;
    if (v->low == v->dfn) {
        qsort(&STACK[pos], top - pos, sizeof(tk_instance_t *), cmp_instance);

        unsigned int j, idx;
        for (j = pos; j < top; ++j) {
            w = STACK[j];
            while (NULL == w->cmds)
                nano_sleep(1000 * 1000);
            for (idx = 0; idx < w->cmds_count; ++idx) {
                char * val = execute_command(&(w->cmds[idx]), r->statemachine);
#ifndef DEBUG_EXEC
                if ((r->flag & REPLY_MASK) && w->lb && w->lb->clientProposals) {
                    /*
                     * call API: replyProposal(w->lb->clientProposals[idx].id, ...args)
                     */
                }
#endif
#ifdef DEBUG_EXEC
                if (r->flag & REPLY_MASK) {
                    if (w->cmds[idx].opcode == GET)
                        printf("REPLY COMMAND: GET %s %s\n", w->cmds[idx].key, val == NULL ? "unknown" : val);
                    else
                        printf("REPLY COMMAND: PUT %s %s\n", w->cmds[idx].key, val == NULL ? "unknown" : val);
                }
#endif
            }
            w->flag |= EXECUTED_MASK;
#ifdef DEBUG_EXEC
            printf("EXECUTED %d COMMANDS\n", w->cmds_count);
#endif
        }
        top = pos;
    }
    return 1;
}

uint8_t
find_SCC(replica_server_param_t * r, tk_instance_t * v)
{
    unsigned short index = 1;
    top = 0;
    return strong_connect(r, v, &index);
}

uint8_t
execute_instance(replica_server_param_t * r, int replica, uint64_t instance)
{
    tk_instance_t * v = &(r->InstanceMatrix[replica][instance]);
    if (!v)
        return 0;
    if (!(v->flag & USED_MASK))
        return 0;
    if (v->flag & EXECUTED_MASK)
        return 1;
    if (!(v->flag & COMMITTED_MASK))
        return 0;
    if (!find_SCC(r, v))
        return 0;
    return 1;
}

char *
execute_command(tk_command_t * c, Tkdatabase_t * st) {
    char * res = NULL;
    switch (c->opcode) {
        case PUT:
            if (1 == putData_into_db(st, c->key, c->val))
                res = c->val;
            break;
        case GET:
            getdata_from_db(st, c->key, &res);
            c->val = res;
            break;
        default:
            return NULL;
    }
    return res;
}

void execute_thread(replica_server_param_t * r) {

    if (!r) {
        info(stderr, "initialize replica first!\n");
        return;
    }

    // only initialized once
    instance_stack_size = 100;
    instance_stack = (tk_instance_t **) malloc(instance_stack_size * sizeof(tk_instance_t *));

    if (!instance_stack) {
        info(stderr, "cannot allocate space for STACK\n");
        return;
    }

    int64_t * problemInstance = (int64_t *) malloc (sizeof(int64_t) * r->group_size);
    uint64_t * timeout = (uint64_t *) malloc (sizeof(uint64_t) * r->group_size);
    uint8_t q, executed;
    uint64_t inst;

    for (q = 0; q < r->group_size; ++q) {
        problemInstance[q] = -1;
        timeout[q] = 0;
    }

    while (true) {
        executed = 0;
        if ((r->flag & SHUTDOWN_MASK) != 0)
            break;
        for (q = 0; q < r->group_size; ++q) {
            for (inst = r->executeupto[q] + 1; inst <= r->MaxInstanceNum[q]; ++inst) {
                if ((r->InstanceMatrix[q][inst].flag & USED_MASK) &&
                    (r->InstanceMatrix[q][inst].flag & EXECUTED_MASK)) {
                    if (inst == r->executeupto[q] + 1)
                        r->executeupto[q] = inst;
                    continue;
                }
                if ((r->InstanceMatrix[q][inst].flag & USED_MASK) == 0 ||
                    (r->InstanceMatrix[q][inst].flag & COMMITTED_MASK) == 0) {
                    if (inst == problemInstance[q]) {
                        timeout[q] += SLEEP_TIME_NS;
                        if (timeout[q] >= COMMIT_GRACE_PERIOD) {
                            /*
                             * start recovery phase for instanceMatrix[q][inst]
                             */
#ifdef DEBUG_EXEC
                            printf("start recovery phase for replica %d, instance %d\n", q, inst);
                            r->InstanceMatrix[q][inst].flag |= USED_MASK;
                            r->InstanceMatrix[q][inst].flag |= COMMITTED_MASK;
#endif
                            timeout[q] = 0;
                        }
                    } else {
                        problemInstance[q] = inst;
                        timeout[q] = 0;
                    }
                    if (!(r->InstanceMatrix[q][inst].flag & USED_MASK))
                        continue;
                    break;
                }
                if (execute_instance(r, q, inst)) {
                    executed = 1;
                    if (inst == r->executeupto[q] + 1)
                        r->executeupto[q] = inst;
                }
            }
        }
        if (!executed)
            nano_sleep(SLEEP_TIME_NS);
    }
}
