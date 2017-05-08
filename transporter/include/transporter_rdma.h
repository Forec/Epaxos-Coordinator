//
// Created by jingle on 17-4-30.
//

#ifndef TKDATABASE_TRANSPORTER_RDMA_H
#define TKDATABASE_TRANSPORTER_RDMA_H

#include <assert.h>
#include <stdio.h>
#define _XOPEN_SOURCE 600   /* for posix_memalign */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <infiniband/verbs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>


#define DEPTH 100
#define NUM_RTTS 100
#define TICKS_PER_USEC 2400

#define CHECK(val, msg) \
        if(val) {printf("val: %d %s\n", val, msg);\
        exit(-1);}


#define USE_RC 1

#define ADDR "192.168.50.11"
#define PORT_NUM 20112


struct exchange_params {
    int lid;
    int qpn;
    int psn;
};

typedef struct exchange_params exchange_params_t;

struct remote_mem {

    uint64_t addr;
    uint32_t rkey;
    uint32_t size;

};

typedef struct remote_mem remote_mem_t;

struct replica_rdma {

    struct ibv_context *context;
    struct ibv_pd *pd;
    struct ibv_cq **send_cq;
    struct ibv_cq **receive_cq;
    struct ibv_qp **qp;
    uint8_t replica_index;
    exchange_params_t *local_qp_attr;

    //struct qp_attr *remote_qp_attr;

    //remote_mem_t *r_mem_server;
    //remote_mem_t *r_mem_client;
    int machine_num;
    int ib_port_base;
    void **buf;
    size_t buf_size;
    void **send_buf;
    size_t send_buf_size;
    void **receive_buf;
    size_t receive_buf_size;
    struct ibv_mr **mr;
    char **addr;
    int tcp_port;
    int port_cnt;
    pthread_t *receive_threads;

};
typedef struct replica_rdma replica_rdma_t;
struct receive_params{
    replica_rdma_t *replica_rdma;
    int from_server_idx;
};
typedef struct receive_params receive_params_t;

int probe_rdma_device(struct ibv_device ***list);
void free_device_list(struct ibv_device **list);
void open_device(struct ibv_device *dev, replica_rdma_t* replica_rdma);
int close_device(replica_rdma_t* replica_rdma);
void create_qp(replica_rdma_t* replica_rdma);
void init_qp(replica_rdma_t *replica_rdma);
void init_qp_attr(replica_rdma_t *replica_rdma);
void init_rdma_struct(replica_rdma_t *replica_rdma);
void free_rdma_struct(replica_rdma_t *replica_rdma);
//void connect_replicas (replica_rdma_t *replica_rdma);





//uint64_t  rtt_times[NUM_RTTS];

//uint64_t rdtsc();

int ibGetLID(struct ibv_context *ctxt, int port);

struct ibv_device *ibFindDevice(const char *name);
int replica_reg_mem(replica_rdma_t *replica_rdma);
void ibPostReceive(struct ibv_qp *qp, struct ibv_mr *mr, void *rxbuf, size_t rxbufsize);
void ibPostSend(struct ibv_qp *qp, struct ibv_mr *mr, void *txbuf, size_t txbufsize);
void ibPostSendAndWait(struct ibv_qp *qp, struct ibv_mr *mr, void *txbuf, size_t txbufsize, struct ibv_cq *cq);
struct exchange_params client_exchange(const char *server, uint16_t port, struct exchange_params *params);
struct exchange_params server_exchange(uint16_t port, struct exchange_params *params);

//  main inteface about RDMA communication;
int rdma_connect_policy(replica_rdma_t *replica_rdma);  //  each replica server will connect the other server that index less then itself, and wait for the greater to connect it;
int connect_init(replica_rdma_t *replica_rdma);  // init the rdma struct, and exchange info;
void *server_msg_receive_thread (void* param);  // the  receive thread;
int server_listen(replica_rdma_t *replica_rdma); // each server will lanch serval some threads to receive msg from other servers;
void close_listenning(int server_self);    // close listening;
int server_send_msg(replica_rdma_t *replica_rdma, void *msg, int len, int server_idx); // send msg;



#endif //TKDATABASE_TRANSPORTER_RDMA_H
