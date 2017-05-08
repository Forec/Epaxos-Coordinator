
gcc rdma_module_test_client.c ../src/transporter_rdma.c -o test_client -lpthread -libverbs -g

gcc rdma_module_test_server.c ../src/transporter_rdma.c -o test_server -lpthread -libverbs -g