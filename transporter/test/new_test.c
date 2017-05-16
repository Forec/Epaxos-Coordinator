//
// Created by root on 17-5-15.
//





#include "../include/transporter_rdma.h"


/*
 * test case :
 *      0. connection;
 *      1. send msg;
 *      2. receive_msg;
 *
 *
 */


rdma_handler_t* init_rdma_client()
{
    char *addr = "tfs01";
    int tcp_port = 18811;
    rdma_handler_t *handler = rdma_connect(addr, tcp_port);
    return handler;
}

rdma_handler_t* init_rdma_server()
{
    int tcp_port = 18811;
    rdma_handler_t *handler = rdma_accept(tcp_port);
    return handler;
}


int main(int argc, char* arg[])

{
    const char* servername=arg[1];
    rdma_handler_t *handler = NULL;
    if(servername) {
        handler = init_rdma_client();
    } else{
        handler = init_rdma_server();
    }
    char *buf = "12345678";
    if(servername){
        sleep(1);
        rdma_send(handler, (void*) buf, 8);
        destroy_rdma_connect(handler);
    }else{

        while(1){
            void *rbuf=(void *)malloc(sizeof(void) * 8);
            rdma_receive(handler, rbuf, 8);
            printf("%s\n", (char *)rbuf);
        }
        // destroy_rdma_connect(handler);

    }


}
