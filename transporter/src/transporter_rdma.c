//
// Created by jingle on 17-4-30.
//

#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include "../include/transporter_rdma.h"




//#define DEPTH 100
#define THREADS_NUM 2
#define BUF_SIZE  2 * 8;


uint64_t rdtsc()
{
	uint32_t lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return (((uint64_t)hi << 32) | lo);
}


int probe_rdma_device(struct ibv_device ***list){

	int num;
	*list = ibv_get_device_list(&num);
	if(*list == NULL) {
		return 0;
	}
	return num;
}
void free_device_list(struct ibv_device **list){

	ibv_free_device_list(list);

}

void open_device(struct ibv_device *dev, replica_rdma_t* replica_rdma){

	replica_rdma->context = ibv_open_device(dev);
	CHECK(!replica_rdma->context,  "can't open the device");
	struct ibv_device_attr device_attr;
	ibv_query_device(replica_rdma->context, &device_attr);

	//    replica_rdma->port_cnt = device_attr.phys_ports_cnt;

}

int close_device(replica_rdma_t *replica_rdma){

	return ibv_close_device(replica_rdma->context);

}


void create_qp(replica_rdma_t *replica_rdma){

	int num;
	num = replica_rdma->machine_num;
	replica_rdma->send_cq = (struct ibv_cq **)malloc(sizeof(struct ibv_cq *) * num);
	replica_rdma->receive_cq = (struct ibv_cq **)malloc(sizeof(struct ibv_cq *) * num);
	replica_rdma->qp = (struct ibv_qp **)malloc(sizeof(struct ibv_qp *) *num);
	int i;
	struct ibv_qp_init_attr qp_init_attr;
	for(i = 0; i < num; i++){
		if(i == replica_rdma->replica_index){
			continue;
		}
		replica_rdma->send_cq[i] = ibv_create_cq(replica_rdma->context, DEPTH, NULL, NULL, 0);
		if(replica_rdma->send_cq[i] == NULL){
			printf("create cq failed!\n");
		}
		CHECK(!replica_rdma->send_cq[i], "Can't create send completion queue!");
		replica_rdma->receive_cq[i] = ibv_create_cq(replica_rdma->context, DEPTH, NULL, NULL, 0);
		if(replica_rdma->receive_cq[i] == NULL){
			printf("create cq failed!\n");
		}
		CHECK(!replica_rdma->receive_cq[i], "Can't create receive completion queue!");
		memset(&qp_init_attr, 0, sizeof(qp_init_attr));
		qp_init_attr.send_cq = replica_rdma->send_cq[i];
		qp_init_attr.recv_cq = replica_rdma->receive_cq[i];
		qp_init_attr.qp_type = IBV_QPT_RC;
		qp_init_attr.cap.max_send_wr = DEPTH;
		qp_init_attr.cap.max_recv_wr = DEPTH;
		qp_init_attr.cap.max_send_sge = 1;
		qp_init_attr.cap.max_recv_sge = 1;
		qp_init_attr.cap.max_inline_data = 0;
		qp_init_attr.sq_sig_all = 0;
		replica_rdma->qp[i] = ibv_create_qp(replica_rdma->pd, &qp_init_attr);

		CHECK(!replica_rdma->qp[i], "Can not create queue pair!");
	}
}

void init_qp(replica_rdma_t *replica_rdma){

	int number;
	number = replica_rdma->machine_num;
	struct ibv_qp_attr attr;

	memset(&attr, 0, sizeof(attr));
	//printf("bug here??\n");
	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.qp_access_flags=IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE;
	int i;
	for(i = 0; i < number; i++){
	//	printf("bug here??\n");
		if(i == replica_rdma->replica_index){
			continue;
		}
		attr.port_num = 1;
		//printf("replica_rdma->ib_port_base: %d\n", replica_rdma->ib_port_base);
		CHECK(ibv_modify_qp(replica_rdma->qp[i],&attr,
					IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS),"Cannot modify queue pair.");
	}
}

void init_qp_attr(replica_rdma_t *replica_rdma){

	int number;
	number = replica_rdma->machine_num;
	replica_rdma->local_qp_attr = (exchange_params_t *)malloc(sizeof(exchange_params_t) * number);
	int i;
	for(i = 0; i < number; i++){
		replica_rdma->local_qp_attr[i].qpn = replica_rdma->qp[i]->qp_num;
		replica_rdma->local_qp_attr[i].lid = ibGetLID(replica_rdma->context, 1);
		replica_rdma->local_qp_attr[i].psn = lrand48() & 0xffffffff + i;
	}
}


void init_rdma_struct(replica_rdma_t *replica_rdma){
	//replica_rdma->pd = ibv_alloc_pd(replica_rdma->context);
	CHECK(!replica_rdma->pd, "Can't allocate protection domain.");
	create_qp(replica_rdma);
	printf("create qp success !\n");
	init_qp(replica_rdma);
	printf("init_qp success !\n");
	init_qp_attr(replica_rdma);
	printf("init_qp_attr success !\n");

}

void free_rdma_struct(replica_rdma_t *replica_rdma){
	int i;
	int number;
	number = replica_rdma->machine_num;
	ibv_dealloc_pd(replica_rdma->pd);
	free(replica_rdma->local_qp_attr);
	for(i = 0; i < number; i++){
		//ibv_destory_cq
		if(i == replica_rdma->replica_index){
			continue;
		}
		ibv_destroy_cq(replica_rdma->receive_cq[i]);
		ibv_destroy_cq(replica_rdma->send_cq[i]);
		ibv_destroy_qp(replica_rdma->qp[i]);
	}
	//number = replica_rdma->buf_size;
	for(i = 0; i < number; i++){
		if(i == replica_rdma->replica_index){
			continue;
		}
		free(replica_rdma->buf[i]);
		ibv_dereg_mr(replica_rdma->mr[i]);

	}
	free(replica_rdma->receive_cq);
	free(replica_rdma->send_cq);
	free(replica_rdma->qp);
	free(replica_rdma->buf);
	free(replica_rdma->mr);
}

//void register_memory(replica_rdma_t *replica_rdma){
//
//
//
//}
//
//
//void server_send(replica_rdma_t *replica_rdma){
//
//    int sock_fd, accept_fd;
//    int i, j, k;
//    struct sockaddr_in sin;
//    int number = replica_rdma->machine_num;
//    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//    if(sock_fd < 0){
//        printf("%s\n", "Cannot create socket");
//        exit(-1);
//    }
//    memset((char *) &sin, 0, sizeof(sin));
//    sin.sin_family = AF_INET;
//    sin.sin_addr.s_addr = INADDR_ANY;
//    sin.sin_port = htons(PORT_NUM);
//    if(bind(sock_fd, (struct sockaddr *) &sin, sizeof(sin)) < 0){
//        printf("Cannot bind\n");
//        exit(-1);
//    }
//    replica_rdma->remote_qp_attr =(struct qp_attr *)malloc(sizeof(struct qp_attr)* replica_rdma->machine_num);
//
//    listen(sock_fd, replica_rdma->replica_index + 1);
//    int index;
//    for(int i=0; i<replica_rdma->replica_index+1; i++){
//        accept_fd=accept(sock_fd, NULL, NULL);
//        if(accept_fd < 0){
//            printf("Cannot accept\n");
//            exit(-1);
//        }
//        if(read(accept_fd, (char *)(&index), sizeof(int)) < 0){
//            printf("Cannot read the other peer's Index\n");
//        }
//
//    }
//
//
//
//
//}
//
//void connect_replicas (replica_rdma_t *replica_rdma){
//
//    int i;
//    struct ibv_qp_attr conn_attr;
//    int rtr_flags, rts_flags;
//    int offset = replica_rdma->machine_num;
//
//
//
//}



int ibGetLID(struct ibv_context *ctxt, int port)
{
	struct ibv_port_attr ipa;
	if (ibv_query_port(ctxt, port, &ipa)) {
		fprintf(stderr, "ibv_query_port failed\n");
		exit(1);
	}
	return ipa.lid;
}

struct ibv_device *ibFindDevice(const char *name)
{
	struct ibv_device **devices;

	devices = ibv_get_device_list(NULL);
	if (devices == NULL)
		return NULL;

	if (name == NULL)
		return devices[0];

	int i;
	for (i = 0; devices[i] != NULL; i++) {
		if (strcmp(devices[i]->name, name) == 0)
			return devices[i];
	}

	return NULL;
}

void ibPostReceive(struct ibv_qp *qp, struct ibv_mr *mr, void *rxbuf, size_t rxbufsize)
{
	struct ibv_sge isge = { (uint64_t)rxbuf, rxbufsize, mr->lkey };
	struct ibv_recv_wr irwr;

	memset(&irwr, 0, sizeof(irwr));
	irwr.wr_id = 1;
	irwr.next = NULL;
	irwr.sg_list = &isge;
	irwr.num_sge = 1;

	struct ibv_recv_wr *bad_irwr;
	if (ibv_post_recv(qp, &irwr, &bad_irwr)) {
		fprintf(stderr, "failed to ibv_post_recv\n");
		exit(1);
	}
}

void ibPostSend(struct ibv_qp *qp, struct ibv_mr *mr, void *txbuf, size_t txbufsize)
{
	struct ibv_sge isge = { (uint64_t)txbuf, txbufsize, mr->lkey };
	struct ibv_send_wr iswr;

	memset(&iswr, 0, sizeof(iswr));
	iswr.wr_id = 2;
	iswr.next = NULL;
	iswr.sg_list = &isge;
	iswr.num_sge = 1;
	iswr.opcode = IBV_WR_SEND;
	iswr.send_flags = IBV_SEND_SIGNALED;

	struct ibv_send_wr *bad_iswr;
	if (ibv_post_send(qp, &iswr, &bad_iswr)) {
		fprintf(stderr, "ibv_post_send failed!\n");
		exit(1);
	}
}

void ibPostSendAndWait(struct ibv_qp *qp, struct ibv_mr *mr, void *txbuf, size_t txbufsize, struct ibv_cq *cq)
{
	ibPostSend(qp, mr, txbuf, txbufsize);

	struct ibv_wc iwc;
	while (ibv_poll_cq(cq, 1, &iwc) < 1)
		;
	if (iwc.status != IBV_WC_SUCCESS) {
		fprintf(stderr, "ibv_poll_cq returned failure\n");
		exit(1);
	}
}



int server_send_msg(replica_rdma_t *replica_rdma, void *msg, int len, int server_idx)
{
	if(len>replica_rdma->send_buf_size){
		printf("msg is to long\n");
		return 0;
	}
	else {
		memcpy(replica_rdma->send_buf[server_idx], msg, (size_t) len);
	//	replica_rdma->send_buf[server_idx] = msg;
		ibPostSendAndWait(replica_rdma->qp[server_idx], replica_rdma->mr[server_idx],
				replica_rdma->send_buf[server_idx], replica_rdma->send_buf_size, replica_rdma->send_cq[server_idx]);
	}
	return 1;
}

struct exchange_params client_exchange(const char *server, uint16_t port, struct exchange_params *params, remote_mem_t *mem)
{
	int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == -1) {
		perror("socket");
		exit(1);
	}

	struct hostent *hent = gethostbyname(server);
	if (hent == NULL) {
		perror("gethostbyname");
		exit(1);
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = PF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr = *((struct in_addr *)hent->h_addr);

	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("connect");
		exit(1);
	}

	write(s, params, sizeof(*params));
	read(s, params, sizeof(*params));
	write(s, mem, sizeof(*mem));
	read(s, mem, sizeof(*mem));
	close(s);

	return *params;
}

struct exchange_params server_exchange(uint16_t port, struct exchange_params *params, remote_mem_t *mem)
{
	int s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == -1) {
		perror("socket");
		exit(1);
	}

	int on = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = PF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(s, 1) == -1) {
		perror("listen");
		exit(1);
	}

	struct sockaddr_in csin;
	socklen_t csinsize = sizeof(csin);
	int c = accept(s, (struct sockaddr *)&csin, &csinsize);
	if (c == -1) {
		perror("accept");
		exit(1);
	}

	write(c, params, sizeof(*params));
	read(c, params, sizeof(*params));
	write(c, mem, sizeof(*mem));
	read(c, mem, sizeof(*mem));

	close(c);
	close(s);

	return *params;
}


int replica_reg_mem(replica_rdma_t *replica_rdma)
{
	int i;
	int machine_number = replica_rdma->machine_num;
	replica_rdma->buf = (void **)malloc(sizeof(void *) * machine_number);
	replica_rdma->send_buf = (void **)malloc(sizeof(void *) *machine_number);
	replica_rdma->receive_buf = (void **)malloc(sizeof(void *) *machine_number);
	size_t buf_size = replica_rdma->buf_size;
	printf("buf_size: %u\n", buf_size);
	replica_rdma->mr = (struct ibv_mr **)malloc(sizeof(struct ibv_mr *) * machine_number);
	for(i = 0; i < machine_number; i++){
		//printf("replica_index:%d\n", replica_rdma->replica_index);
		if(i == replica_rdma->replica_index){
			continue;
		}
		if(posix_memalign(&replica_rdma->buf[i], 4096, buf_size)){
			fprintf(stderr, "posix_memalign failed\n");
			return 0;
		}
		replica_rdma->send_buf[i] = replica_rdma->buf[i];
		replica_rdma->send_buf_size = buf_size / 2;
		replica_rdma->receive_buf[i] = replica_rdma->buf[i] + buf_size / 2;
		replica_rdma->receive_buf_size = buf_size / 2;
		// register our userspace buffer with the HCA;
		replica_rdma->mr[i] = ibv_reg_mr(replica_rdma->pd, replica_rdma->buf[i], buf_size,
				IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);
		if(replica_rdma->mr[i] == NULL){
			fprintf(stderr, "failed to register memory region  %d\n", i);
			return 0;
		}
	}
	return 1;
}



int rdma_connect_policy(replica_rdma_t *replica_rdma){

	int i;
	int idx = replica_rdma->replica_index;
	for(i = 0; i < idx; i++){
		exchange_params_t params = {replica_rdma->local_qp_attr[i].lid, replica_rdma->local_qp_attr[i].qpn, replica_rdma->local_qp_attr[i].psn};
		params = client_exchange(replica_rdma->addr[i], replica_rdma->tcp_port, &params);
		int j;
		for (j = 0; j < DEPTH; j++){
			ibPostReceive(replica_rdma->qp[i], replica_rdma->mr[i],
					replica_rdma->receive_buf[i], replica_rdma->receive_buf_size);
		}

		struct ibv_qp_attr qpa;
		memset(&qpa, 0, sizeof(qpa));
		qpa.qp_state = IBV_QPS_RTR;
		qpa.path_mtu = IBV_MTU_4096;
		qpa.dest_qp_num = params.qpn;
		qpa.rq_psn = params.psn;
		qpa.max_dest_rd_atomic = 1;
		qpa.min_rnr_timer = 12;
		qpa.ah_attr.is_global = 0;
		qpa.ah_attr.dlid = params.lid;
		qpa.ah_attr.sl = 0;
		qpa.ah_attr.src_path_bits = 0;
		qpa.ah_attr.port_num = 1;

		if (ibv_modify_qp(replica_rdma->qp[i], &qpa, IBV_QP_STATE |
					IBV_QP_AV |
					IBV_QP_PATH_MTU |
					IBV_QP_DEST_QPN |
					IBV_QP_RQ_PSN |
					IBV_QP_MIN_RNR_TIMER |
					IBV_QP_MAX_DEST_RD_ATOMIC)) {
			fprintf(stderr, "failed to modify qp state\n");
			exit(1);
		}

		// now move to RTS
		qpa.qp_state = IBV_QPS_RTS;
		qpa.timeout = 14;
		qpa.retry_cnt = 7;
		qpa.rnr_retry = 7;
		qpa.sq_psn = params.psn;
		qpa.max_rd_atomic = 1;
		if (ibv_modify_qp(replica_rdma->qp[i], &qpa, IBV_QP_STATE |
					IBV_QP_TIMEOUT |
					IBV_QP_RETRY_CNT |
					IBV_QP_RNR_RETRY |
					IBV_QP_SQ_PSN |
					IBV_QP_MAX_QP_RD_ATOMIC)) {
			fprintf(stderr, "failed to modify qp state\n");
			exit(1);
		}

	}

	for(i = idx + 1; i < replica_rdma->machine_num; i++){
		struct exchange_params params = {replica_rdma->local_qp_attr[i].lid, replica_rdma->local_qp_attr[i].qpn, replica_rdma->local_qp_attr[i].psn};
		params = server_exchange(replica_rdma->tcp_port,&params);
		int j;
		for (j = 0; j < DEPTH; j++){
			ibPostReceive(replica_rdma->qp[i], replica_rdma->mr[i],
					replica_rdma->receive_buf[i], replica_rdma->receive_buf_size);
		}
		struct ibv_qp_attr qpa;
		memset(&qpa, 0, sizeof(qpa));
		qpa.qp_state = IBV_QPS_RTR;
		qpa.path_mtu = IBV_MTU_4096;
		qpa.dest_qp_num = params.qpn;
		qpa.rq_psn = params.psn;
		qpa.max_dest_rd_atomic = 1;
		qpa.min_rnr_timer = 12;
		qpa.ah_attr.is_global = 0;
		qpa.ah_attr.dlid = params.lid;
		qpa.ah_attr.sl = 0;
		qpa.ah_attr.src_path_bits = 0;
		qpa.ah_attr.port_num = 1;

		if (ibv_modify_qp(replica_rdma->qp[i], &qpa, IBV_QP_STATE |
					IBV_QP_AV |
					IBV_QP_PATH_MTU |
					IBV_QP_DEST_QPN |
					IBV_QP_RQ_PSN |
					IBV_QP_MIN_RNR_TIMER |
					IBV_QP_MAX_DEST_RD_ATOMIC)) {
			fprintf(stderr, "failed to modify qp state\n");
			exit(1);
		}

		// now move to RTS
		qpa.qp_state = IBV_QPS_RTS;
		qpa.timeout = 14;
		qpa.retry_cnt = 7;
		qpa.rnr_retry = 7;
		qpa.sq_psn = params.psn;
		qpa.max_rd_atomic = 1;
		if (ibv_modify_qp(replica_rdma->qp[i], &qpa, IBV_QP_STATE |
					IBV_QP_TIMEOUT |
					IBV_QP_RETRY_CNT |
					IBV_QP_RNR_RETRY |
					IBV_QP_SQ_PSN |
					IBV_QP_MAX_QP_RD_ATOMIC)) {
			fprintf(stderr, "failed to modify qp state\n");
			exit(1);
		}
	}
	return 1;
}




/*
 *
 *     recevice msg from certain server;
 *
 *
 */

void *server_msg_receive_thread (void* param)
{
	receive_params_t *p = (receive_params_t *)param;
	replica_rdma_t *replica_rdma1 = p->replica_rdma;
	//printf("replica_idx: %d\n", p->replica_rdma->replica_index);
	int idx = p->from_server_idx;
	printf("receive_msg from: %d\n", idx);

	while (1) {
		// receive first
		//printf("receive: %s\n", (char *) replica_rdma1->receive_buf[idx]);
		struct ibv_wc iwc;
		while (ibv_poll_cq(replica_rdma1->receive_cq[idx], 1, &iwc) < 1);
		if (iwc.status != IBV_WC_SUCCESS) {
			fprintf(stderr, "ibv_poll_cq returned failure\n");
			exit(1);
		}
		printf("receive: %s\n", (char *) replica_rdma1->receive_buf[idx]);
		// after receive put a ibpostreceive again

		ibPostReceive(replica_rdma1->qp[idx], replica_rdma1->mr[idx], replica_rdma1->receive_buf[idx], replica_rdma1->receive_buf_size);
		//		memcpy(txbuf, &counter, sizeof(counter));
		//		ibPostSendAndWait(qp, mr, txbuf, sizeof(counter), txcq);
	}
}
/*  ********************************************************************************************************
 *   server receive msg from other replica_server                                                          *
 *  ********************************************************************************************************/
pthread_t threads[THREADS_NUM];
//replica_rdma->receive_threads = &threads;
receive_params_t params[THREADS_NUM];
//

int server_listen(replica_rdma_t *replica_rdma1){

	int i;
	int receive_threads_num = replica_rdma1->machine_num;
       	
	printf("%x\n", replica_rdma1->qp[1]);

//	pthread_t threads[receive_threads_num];
	//replica_rdma->receive_threads = &threads;
//	receive_params_t params[receive_threads_num];
	
	for(i = 0; i < replica_rdma1->machine_num; i++){
		if(i == replica_rdma1->replica_index){
			continue;
		}
		//receive_params_t p;
		//p.replica_rdma = replica_rdma;
		//p.from_server_idx = i;
		params[i].replica_rdma = replica_rdma1;
		printf("i: %d\n", i);
		printf("qp: %x\n", params[i].replica_rdma);
		params[i].from_server_idx = i;
		//params[i].replica_rdma = replica_rdma1;
		//params[i] = p;
		printf("idx: %d\n", params[i].from_server_idx);
		pthread_create(&threads[i], NULL, server_msg_receive_thread, (void *)&params[i]);
	//	pthread_detach(threads[i]);
	}
	//for(i = 0; i < replica_rdma1->machine_num; i++){
	//	pthread_detach(threads[i]);
	//}

	return 1;
}

void close_listenning(int server_self)
{
	int i = 0;
	for(i = 0; i < THREADS_NUM; i++){
		if(i == server_self){
			continue;
		}
		pthread_cancel(threads[i]);
	}
}


int connect_init(replica_rdma_t *replica_rdma){
	struct ibv_device *dev = NULL;
	dev = ibFindDevice(NULL);

	if(dev == NULL){
		fprintf(stderr, "failed to find infiniband device \n");
		return 0;
	}
	printf("Using ib device '%s'. \n", dev->name);
	replica_rdma->context = ibv_open_device(dev);
	if(replica_rdma->context == NULL){
		printf("failed to open infinband device\n");
		return 0;
	}
	replica_rdma->pd = ibv_alloc_pd(replica_rdma->context);
	if(replica_rdma->pd == NULL){
		printf("failed to allocate infinband pd\n");
		return 0;
	}
	printf("connect_init: replica_index: %d\n", replica_rdma->replica_index);
	if(!replica_reg_mem(replica_rdma)){

		printf("register memory failed\n");
		return 0;
	}
	printf("register memory success !\n");
	init_rdma_struct(replica_rdma);
	printf("init_rdma_struct success !\n");
	rdma_connect_policy(replica_rdma);
	return 1;
}

/*
 *
 *
 *      NEW DESIGN FOR RDMA INTERFACE;
 *
 *
 *
 */


int rdma_reg_mem(rdma_handler_t *handler){

    size_t buf_size = handler->buf_size;
    handler->mr = (struct ibv_mr *) malloc(sizeof(struct ibv_mr));
    if(posix_memalign(&handler->buf, 4096, buf_size)){
        fprintf(stderr, "posix_memalign failed\n");
        return 0;
    }
    handler->send_buf = handler->buf;
    handler->send_buf_size = buf_size / 2;
    handler->receive_buf = handler->buf + buf_size / 2;
    handler->receive_buf_size = buf_size / 2;
    // register our userspace buffer with the HCA;
    handler->mr = ibv_reg_mr(handler->pd, handler->buf, buf_size,
                                     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE);

    if(handler->mr == NULL){
        fprintf(stderr, "failed to register memory region\n");
        return 0;
    }
	handler->re_mem = (remote_mem_t *)malloc(sizeof(remote_mem_t));
	handler->re_mem->addr = (uint64_t)handler->buf;
	handler->re_mem->rkey = handler->mr->rkey;
	handler->re_mem->size = (uint32_t)handler->buf_size;
    return 1;
}

void create_qp_for_connection(rdma_handler_t *handler){

    handler->send_cq = (struct ibv_cq *)malloc(sizeof(struct ibv_cq));
    handler->receive_cq = (struct ibv_cq *)malloc(sizeof(struct ibv_cq));
    handler->qp_of_this = (struct ibv_qp *)malloc(sizeof(struct ibv_qp));
    struct ibv_qp_init_attr qp_init_attr;

    handler->send_cq = ibv_create_cq(handler->context, DEPTH, NULL, NULL, 0);
    if(handler->send_cq == NULL){
        printf("create cq failed!\n");
    }
    CHECK(!handler->send_cq, "Can't create send completion queue!");
    handler->receive_cq = ibv_create_cq(handler->context, DEPTH, NULL, NULL, 0);
    if(handler->receive_cq == NULL){
        printf("create cq failed!\n");
    }
    CHECK(!handler->receive_cq, "Can't create receive completion queue!");
    memset(&qp_init_attr, 0, sizeof(qp_init_attr));
    qp_init_attr.send_cq = handler->send_cq;
    qp_init_attr.recv_cq = handler->receive_cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = DEPTH;
    qp_init_attr.cap.max_recv_wr = DEPTH;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;
    qp_init_attr.cap.max_inline_data = 0;
    qp_init_attr.sq_sig_all = 0;
    handler->qp_of_this = ibv_create_qp(handler->pd, &qp_init_attr);

    CHECK(!handler->qp_of_this, "Can not create queue pair!");

}

void init_qp_for_connection(rdma_handler_t *handler){
    struct ibv_qp_attr attr;

    memset(&attr, 0, sizeof(attr));
    //printf("bug here??\n");
    attr.qp_state = IBV_QPS_INIT;
    attr.pkey_index = 0;
    attr.qp_access_flags=IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_LOCAL_WRITE;
    attr.port_num = 1;
        //printf("replica_rdma->ib_port_base: %d\n", replica_rdma->ib_port_base);
    CHECK(ibv_modify_qp(handler->qp_of_this, &attr,
                            IBV_QP_STATE|IBV_QP_PKEY_INDEX|IBV_QP_PORT|IBV_QP_ACCESS_FLAGS),"Cannot modify queue pair.");

}

void init_qp_attr_for_connection(rdma_handler_t *handler){
    handler->local_qp_attr = (exchange_params_t *)malloc(sizeof(exchange_params_t));
    handler->local_qp_attr->qpn = handler->qp_of_this->qp_num;
    handler->local_qp_attr->lid = ibGetLID(handler->context, 1);
    handler->local_qp_attr->psn = lrand48() & 0xffffffff;
}


void init_rdma_connection(rdma_handler_t *handler){

    //replica_rdma->pd = ibv_alloc_pd(replica_rdma->context);
    CHECK(!handler->pd, "Can't allocate protection domain.");
    create_qp_for_connection(handler);
    printf("create qp success !\n");
    init_qp_for_connection(handler);
    printf("init_qp success !\n");
    init_qp_attr_for_connection(handler);
    printf("init_qp_attr success !\n");

}

/*
 *   CLIENT SIDE
 *
 */

rdma_handler_t *rdma_connect(const char *addr, int tcp_port){

    rdma_handler_t *handler = (rdma_handler_t *)malloc(sizeof(rdma_handler_t));

    struct ibv_device *dev = NULL;
    dev = ibFindDevice(NULL);
    if(dev == NULL){
        fprintf(stderr, "failed to find infiniband device \n");
        return NULL;
    }
    printf("Using ib device '%s'.", dev->name);
    handler->context = ibv_open_device(dev);
    if(handler->context == NULL){
        printf("failed to open infiniband device\n");
        return NULL;
    }
    handler->pd = ibv_alloc_pd(handler->context);
    if(handler->pd == NULL){
        printf("failed to allocate infiniband protect domain\n");
    }
	handler->buf_size = BUF_SIZE;
    if(!rdma_reg_mem(handler)){
        printf("Register memory failed\n");
        return NULL;
    }
    init_rdma_connection(handler);
    exchange_params_t params = {handler->local_qp_attr->lid, handler->local_qp_attr->qpn, handler->local_qp_attr->psn};
    params = client_exchange(addr, tcp_port, &params);
    int j;
    for (j = 0; j < DEPTH; j++){
        ibPostReceive(handler->qp_of_this, handler->mr,
                      handler->receive_buf, handler->receive_buf_size);
    }

    struct ibv_qp_attr qpa;
    memset(&qpa, 0, sizeof(qpa));
    qpa.qp_state = IBV_QPS_RTR;
    qpa.path_mtu = IBV_MTU_4096;
    qpa.dest_qp_num = params.qpn;
    qpa.rq_psn = params.psn;
    qpa.max_dest_rd_atomic = 1;
    qpa.min_rnr_timer = 12;
    qpa.ah_attr.is_global = 0;
    qpa.ah_attr.dlid = params.lid;
    qpa.ah_attr.sl = 0;
    qpa.ah_attr.src_path_bits = 0;
    qpa.ah_attr.port_num = 1;

    if (ibv_modify_qp(handler->qp_of_this, &qpa, IBV_QP_STATE |
                                                 IBV_QP_AV |
                                                 IBV_QP_PATH_MTU |
                                                 IBV_QP_DEST_QPN |
                                                 IBV_QP_RQ_PSN |
                                                 IBV_QP_MIN_RNR_TIMER |
                                                 IBV_QP_MAX_DEST_RD_ATOMIC)) {
        fprintf(stderr, "failed to modify qp state\n");
        return  NULL;
    }

    // now move to RTS
    qpa.qp_state = IBV_QPS_RTS;
    qpa.timeout = 14;
    qpa.retry_cnt = 7;
    qpa.rnr_retry = 7;
    qpa.sq_psn = params.psn;
    qpa.max_rd_atomic = 1;
    if (ibv_modify_qp(handler->qp_of_this, &qpa, IBV_QP_STATE |
                                                 IBV_QP_TIMEOUT |
                                                 IBV_QP_RETRY_CNT |
                                                 IBV_QP_RNR_RETRY |
                                                 IBV_QP_SQ_PSN |
                                                 IBV_QP_MAX_QP_RD_ATOMIC)) {
        fprintf(stderr, "failed to modify qp state\n");
        return NULL;
    }

    return handler;
}

/*
 *   SERVER SIDE
 *
 *
 */


/*
 *
 *  This Function is just like tcp accept, but do not need for(;;); it contains a while(1);
 *
 */

rdma_handler_t * rdma_accept(int tcp_port){
    rdma_handler_t *handler = (rdma_handler_t *)malloc(sizeof(rdma_handler_t));

    struct ibv_device *dev = NULL;
    dev = ibFindDevice(NULL);
    if(dev == NULL){
        fprintf(stderr, "failed to find infiniband device \n");
        return NULL;
    }
    printf("Using ib device '%s'.\n", dev->name);
    handler->context = ibv_open_device(dev);
    if(handler->context == NULL){
        printf("failed to open infiniband device\n");
        return NULL;
    }
    handler->pd = ibv_alloc_pd(handler->context);
	handler->buf_size = BUF_SIZE;
    if(handler->pd == NULL){
        printf("failed to allocate infiniband protect domain\n");
    }
    if(!rdma_reg_mem(handler)){
        printf("Register memory failed\n");
        return NULL;
    }
    init_rdma_connection(handler);
    exchange_params_t params = {handler->local_qp_attr->lid, handler->local_qp_attr->qpn, handler->local_qp_attr->psn};
    params = server_exchange(tcp_port,&params);
    int j;
    for (j = 0; j < DEPTH; j++){
        ibPostReceive(handler->qp_of_this, handler->mr,
                      handler->receive_buf, handler->receive_buf_size);
    }

    struct ibv_qp_attr qpa;
    memset(&qpa, 0, sizeof(qpa));
    qpa.qp_state = IBV_QPS_RTR;
    qpa.path_mtu = IBV_MTU_4096;
    qpa.dest_qp_num = params.qpn;
    qpa.rq_psn = params.psn;
    qpa.max_dest_rd_atomic = 1;
    qpa.min_rnr_timer = 12;
    qpa.ah_attr.is_global = 0;
    qpa.ah_attr.dlid = params.lid;
    qpa.ah_attr.sl = 0;
    qpa.ah_attr.src_path_bits = 0;
    qpa.ah_attr.port_num = 1;

    if (ibv_modify_qp(handler->qp_of_this, &qpa, IBV_QP_STATE |
                                                 IBV_QP_AV |
                                                 IBV_QP_PATH_MTU |
                                                 IBV_QP_DEST_QPN |
                                                 IBV_QP_RQ_PSN |
                                                 IBV_QP_MIN_RNR_TIMER |
                                                 IBV_QP_MAX_DEST_RD_ATOMIC)) {
        fprintf(stderr, "failed to modify qp state\n");
        return  NULL;
    }

    // now move to RTS
    qpa.qp_state = IBV_QPS_RTS;
    qpa.timeout = 14;
    qpa.retry_cnt = 7;
    qpa.rnr_retry = 7;
    qpa.sq_psn = params.psn;
    qpa.max_rd_atomic = 1;
    if (ibv_modify_qp(handler->qp_of_this, &qpa, IBV_QP_STATE |
                                                 IBV_QP_TIMEOUT |
                                                 IBV_QP_RETRY_CNT |
                                                 IBV_QP_RNR_RETRY |
                                                 IBV_QP_SQ_PSN |
                                                 IBV_QP_MAX_QP_RD_ATOMIC)) {
        fprintf(stderr, "failed to modify qp state\n");
        return NULL;
    }

    return handler;
}

/*
 *
 *  send and receive;
 *
 */

int rdma_receive(rdma_handler_t* handler, void *buf, int32_t len){

    struct ibv_wc iwc;
    while (ibv_poll_cq(handler->receive_cq, 1, &iwc) < 1);
    if (iwc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "ibv_poll_cq returned failure\n");
        return 0;
    }
    //printf("receive: %s\n", (char *) replica_rdma1->receive_buf[idx]);
    // after receive put a ibpostreceive again
	//printf("receive_buf_size: %d, ")
    //assert(handler->receive_buf_size != (size_t) len);
    memcpy(buf, handler->receive_buf, handler->receive_buf_size);
    ibPostReceive(handler->qp_of_this, handler->mr, handler->receive_buf, handler->receive_buf_size);
    //		memcpy(txbuf, &counter, sizeof(counter));
    //		ibPostSendAndWait(qp, mr, txbuf, sizeof(counter), txcq);


}

int rdma_send(rdma_handler_t* handler, void *buf, int32_t len){
    if(len>handler->send_buf_size){
        printf("msg is to long\n");
        return 0;
    }
    else {
        memcpy(handler->send_buf, buf, (size_t)len);
        ibPostSendAndWait(handler->qp_of_this, handler->mr,
                          handler->send_buf, handler->send_buf_size, handler->send_cq);
    }
    return 1;
}



/*
 * rdma disconnect
 */

void destroy_rdma_connect(rdma_handler_t *handler){
    ibv_dealloc_pd(handler->pd);
    free(handler->local_qp_attr);

    ibv_destroy_cq(handler->receive_cq);
    ibv_destroy_cq(handler->send_cq);
    ibv_destroy_qp(handler->qp_of_this);
    //number = replica_rdma->buf_size;
    free(handler->buf);
    free(handler->send_buf);
    free(handler->receive_buf);
    ibv_dereg_mr(handler->mr);
    free(handler->receive_cq);
    free(handler->send_cq);
    free(handler->qp_of_this);
   // free(replica_rdma->buf);
    free(handler->mr);
    free(handler);
}



/* some thing changed after read code of forec */

int sendData(rdma_handler_t *handler, void *buf, size_t buf_len){


}

int readUntil(rdma_handler_t *handler, void *buf, size_t buf_len){

}

