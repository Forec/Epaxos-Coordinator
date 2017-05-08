//
// Created by jingle on 17-5-6.
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


void init_rdma(replica_rdma_t *replica_rdma)
{
    int nums = 2;
    replica_rdma->replica_index = 1;
    replica_rdma->machine_num = nums;
    replica_rdma->ib_port_base = 1;
    replica_rdma->buf_size = 8 * 2;
    replica_rdma->addr = (char **)malloc(sizeof(char *) * nums);
    int i;
    char *addr1 = "tfs01";
    char *addr2 = "tfs02";
    replica_rdma->addr[0] = addr1;
    replica_rdma->addr[1] = addr2;
    replica_rdma->tcp_port = 18115;

}


int main()

{
    replica_rdma_t *replica_rdma = (replica_rdma_t *)malloc(sizeof(replica_rdma_t));
    init_rdma(replica_rdma);
    printf("replica:%d\n", replica_rdma->replica_index);
    if(!connect_init(replica_rdma)){
        printf("connect_init failed\n");
    };
    if(!server_listen(replica_rdma)){
	printf("server_listen failed\n");
    }
    printf("server_listen success \n");
    
    sleep(10); 
   
    char *msg = "8654321";
    int i;
    for(i = 0; i < 10; i++){
	server_send_msg(replica_rdma, (void *)msg, 8, 0);
    }
    //server_send_msg(replica_rdma, (void *)msg, 8, 0);
    printf("send_msg success!\n");
    sleep(5);
    printf("begin to destroy!\n"); 
    close_listenning(replica_rdma->replica_index);
    printf("close_listenning complete\n");
    free_rdma_struct(replica_rdma);
    return 0;

}
