//
// Created by forec on 17-4-20.
//

#include "exec.h"


void nano_sleep(unsigned int ns){
    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = ns;
    nanosleep(&ts, 0);
}

int cmp_instance(const void *p1, const void *p2) {
    return ((tk_instance_t *)p1)->seq - ((tk_instance_t *)p2)->seq;
}

static uint8_t
strong_connect(replica_server_param_t * r,
               tk_instance_t * v,
               unsigned short * index)
{
    v->dfn = *index;
    v->low = *index;
    (*index)++;

    if (instance_stack_size == top){
        if (!realloc(instance_stack, instance_stack_size >> 1)){
            info(stderr, "cannot realloc space for instance stack in tarjan.\n");
            return 0;
        };
        instance_stack_size >>= 1;
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
            tk_instance_t * w = &(r->InstanceMatrix[q][i)];

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
            } else if (w->dfn < v->dfn) {
                v->low = w->dfn;
            }
        }
    }

    tk_instance_t * w;
    if (v->low == v->dfn) {
        qsort(STACK + pos, top - pos, sizeof(tk_instance_t *), cmp_instance);
        int j, idx;
        for (j = pos; j < top; ++j) {
            w = STACK[j];
            while (NULL == w->cmds)
                nano_sleep(1000 * 1000);
            for (idx = 0; idx < w->cmds_count; ++idx) {
                int val = execute_command(w->cmds[idx], r->statemachine);
                if (r->enablereply && w->lb && w->lb->clientProposals) {
                    /*
                     * call API: replyProposal(w->lb->clientProposals[idx].id, ...args)
                     */
                }
            }
            w->flag |= EXECUTED_MASK;
        }
        top = pos;
    }
    return 1;
}

static uint8_t
find_SCC(replica_server_param_t * r, tk_instance_t * v)
{
    unsigned short index = 1;
    top = 0;
    return strong_connect(r, v, &index);
}

static uint8_t
execute_instance(replica_server_param_t * r, int replica, int instance)
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

static char *
execute_command(tk_command_t c, Tkdatabase_t * st) {

}