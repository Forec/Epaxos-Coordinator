//
// Created by forec on 17-4-20.
//

#include "../include/exec.h"

//#define DEBUG_EXEC

unsigned int instance_stack_size;
tk_instance ** instance_stack;
unsigned int top;

int cmp_instance(const void *p1, const void *p2) {
    if ((*(tk_instance **)p1)->seq <  (*(tk_instance **)p2)->seq) return -1;
    if ((*(tk_instance **)p1)->seq == (*(tk_instance **)p2)->seq) return 0;
    return 1;
}

bool
strong_connect(Replica * r,
               tk_instance * v,
               unsigned short * index) {
    v->dfn = *index;
    v->low = *index;
    (*index)++;

    if (instance_stack_size == top){
        if (!realloc(STACK, (instance_stack_size << 1) * sizeof(tk_instance *))){
            info(stderr, "cannot realloc space for instance stack in tarjan.\n");
            return false;
        };
        instance_stack_size <<= 1;
    }
    unsigned int pos = top;
    STACK[top++] = v;

    unsigned int q;
    for (q = 0; q < r->group_size; ++q) {
        int inst = v->deps[q];
        for (int i = r->executeUpTo[q] + 1; i <= inst; ++i) {
            while ((r->InstanceMatrix[q][i] == nullptr) ||
                   r->InstanceMatrix[q][i]->cmds.empty() ||
                   v->cmds.empty()) {
                nano_sleep(1000 * 1000);
            }
            if (r->InstanceMatrix[q][i]->status == EXECUTED)
                continue;
            while (r->InstanceMatrix[q][i]->status != COMMITTED) {
                nano_sleep(1000 * 1000);
            }
            tk_instance * w = r->InstanceMatrix[q][i];

            if (!w->dfn) {
                if (!strong_connect(r, w, index)) {
                    int j;
                    for (j = pos; j < top; ++j)
                        STACK[j]->dfn = 0;
                    top = pos;
                    return false;
                }
                if (w->low < v->low)
                    v->low = w->low;
            } else if (w->dfn < v->low) {
                v->low = w->dfn;
            }
        }
    }

    tk_instance * w;
    if (v->low == v->dfn) {
        qsort(&STACK[pos], top - pos, sizeof(tk_instance *), cmp_instance);

        for (int j = pos; j < top; ++j) {
            w = STACK[j];
            while (w == nullptr || w->cmds.empty())
                nano_sleep(1000 * 1000);
            for (int idx = 0; idx < w->cmds.size(); ++idx) {
                char * val = execute_command(&(w->cmds[idx]), r->statemachine);
#ifndef DEBUG_EXEC
                if (r->Dreply && w->lb && !w->lb->clientProposals.empty()) {
                    /*
                     * call API: replyProposal(w->lb->clientProposals[idx].id, ...args)
                     */
                }
#endif
#ifdef DEBUG_EXEC
                if (r->Restore) {
                    if (w->cmds[idx].opcode == GET)
                        printf("REPLY COMMAND: GET %s %s\n", w->cmds[idx].key, val == nullptr ? "unknown" : val);
                    else
                        printf("REPLY COMMAND: PUT %s %s\n", w->cmds[idx].key, val == nullptr ? "unknown" : val);
                }
#endif
            }
            w->status = EXECUTED;
#ifdef DEBUG_EXEC
            printf("EXECUTED %d COMMANDS\n", w->cmds.size());
#endif
        }
        top = pos;
    }
    return true;
}

bool
find_SCC(Replica * r, tk_instance * v) {
    unsigned short index = 1;
    top = 0;
    return strong_connect(r, v, &index);
}

bool
execute_instance(Replica * r, int replica, int instance)  {
    tk_instance * v = r->InstanceMatrix[replica][instance];
    if (!v)
        return false;
    if (v->status == EXECUTED)
        return true;
    if (v->status != COMMITTED)
        return false;
    return find_SCC(r, v);
}

char *
execute_command(tk_command * c, Tkdatabase * st) {
    char * res = nullptr;
    switch (c->opcode) {
        case PUT:
            if (st->put(c->key, c->val, c->valSize))
                res = c->val;
            break;
        case GET:
            c->val = st->fetch(c->key, c->valSize);
            break;
        default:
            return nullptr;
    }
    return res;
}

void * execute_thread(void * arg) {
    Replica * r = (Replica *)arg;

    if (!r) {
        info(stderr, "initialize replica first!\n");
        return (void*)NULL;
    }

    // only initialized once
    instance_stack_size = 100;
    instance_stack = (tk_instance **) malloc(instance_stack_size * sizeof(tk_instance *));

    if (!instance_stack) {
        info(stderr, "cannot allocate space for STACK\n");
        return (void*)NULL;
    }

    int32_t * problemInstance = new int32_t[r->group_size];
    uint64_t * timeout = new uint64_t[r->group_size];

    for (int q = 0; q < r->group_size; ++q) {
        problemInstance[q] = -1;
        timeout[q] = 0;
    }

    while (!r->Shutdown) {
        bool executed = false;
        for (int q = 0; q < r->group_size; ++q) {
            for (int inst = r->executeUpTo[q] + 1; inst <= r->crtInstance[q]; ++inst) {
                if ((r->InstanceMatrix[q][inst] != nullptr) &&
                    (r->InstanceMatrix[q][inst]->status == EXECUTED)) {
                    if (inst == r->executeUpTo[q] + 1)
                        r->executeUpTo[q] = inst;
                    continue;
                }
                if (r->InstanceMatrix[q][inst]== nullptr ||
                    r->InstanceMatrix[q][inst]->status != COMMITTED) {
                    if (inst == problemInstance[q]) {
                        timeout[q] += SLEEP_TIME_NS;
                        if (timeout[q] >= COMMIT_GRACE_PERIOD) {
                            /*
                             * start recovery phase for instanceMatrix[q][inst]
                             */
#ifdef DEBUG_EXEC
                            printf("start recovery phase for replica %d, instance %d\n", q, inst);
                            r->InstanceMatrix[q][inst].status = COMMITTED;
#endif
                            timeout[q] = 0;
                        }
                    } else {
                        problemInstance[q] = inst;
                        timeout[q] = 0;
                    }
                    if (r->InstanceMatrix[q][inst] == nullptr)
                        continue;
                    break;
                }
                if (execute_instance(r, q, inst)) {
                    executed = true;
                    if (inst == r->executeUpTo[q] + 1)
                        r->executeUpTo[q] = inst;
                }
            }
        }
        if (!executed)
            nano_sleep(SLEEP_TIME_NS);
    }
    delete [] problemInstance;
    delete [] timeout;
    return (void*) nullptr;
}
