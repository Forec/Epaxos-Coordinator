## 通信接口
* `RDMA_CONNECTION` 类型：Replica 建立监听的时候返回的句柄，以及 Dial 到其它 Replica 时返回的句柄。（必须）举例如下，其中变量类型、变量名、方法名等均自定义，也不一定就是指针。
  * 举例：`RDMA_CONNECTION * listener = RDMA::Listen("127.0.0.1");`
  * 举例：`for (;;) { RDMA_CONNECTION * conn = listener->accept(); }`
  * 举例：`RDMA_CONNECTION * conn = RDMA::Dial("127.0.0.1");` 
* 初始化连接：如上面例子中 `RDMA::Listen`、`RDMA::Dial` 等这样的方法。（必须）我建议可以把 `RDMA_CONNECTION` 做成一个类或者结构，增加一些字段比如 `bool error` 用来保存上一次操作是否有错误发生等信息。
* 销毁连接。（必须）
* `int32_t RDMA_WRITE(RDMA_CONNECTION handle, char * buf, int32_t len)`：写句柄，向 handle 指代的 rdma 连接写入从 buf 开始的 len 个字节。返回值为实际发送的字节数量。（必须）
* `int32_t RDMA_READ(RDMA_CONNECTION handle, char * buf, int32_t len)`：读句柄，从 handle 指代的 rdma 连接读取至多 len 个字节，拷贝到 buf 开始的地址处。返回值为实际读取的字节数量。（必须）
* `bool RDMA_WRITE(RDMA_CONNECTION handle, char * buf, int32_t len)`：写句柄，向 handle 指代的 rdma 连接写入从 buf 开始的 len 个字节。必须将 len 个字节完全发送完成后返回 true，若发送过程中出现错误则返回 false。发送过程中如果有资源占用则阻塞直到占用解除。（可选接口）