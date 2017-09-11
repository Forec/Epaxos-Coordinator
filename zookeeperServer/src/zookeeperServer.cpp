//
// Created by forec on 17-5-8.
//

#include "../include/zookeeperServer.h"
#include "../include/serverCnxn.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>

#define maxConnections 100
#define RequestHeaderLength 8


void processConnectRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int32_t socketfd){
    std::cout<<"process connect request"<<std::endl;
    serverCnxnFactory * cnxnFactory = &zServer->cnxnFactory;
    struct ConnectRequest cr;
    struct iarchive *ia;
    ia = create_buffer_iarchive(buf,len);
    deserialize_ConnectRequest(ia,"connectRequest",&cr);
    close_buffer_iarchive(&ia);
    int sessionTimeout = cr.timeOut;
    if (sessionTimeout < cnxnFactory->minSessionTimeout) {
        sessionTimeout = cnxnFactory->minSessionTimeout;
    }
    if (sessionTimeout > cnxnFactory->maxSessionTimeout) {
        sessionTimeout = cnxnFactory->maxSessionTimeout;
    }
    //need to disable socket before session is created
    int64_t sessionId = cr.sessionId;
    if(sessionId != 0){
        //renew a session
        cout<<"session Id ="<<sessionId<<endl;
        bool valid = addSession(&cnxnFactory->st,sessionId,cr.timeOut);
        struct ConnectResponse response;
        if(valid){
            response.timeOut = cnxnFactory->tick;
            struct buffer passwd;
            passwd.len = 16;
            passwd.buff = (char *)malloc(16);
            memcpy(passwd.buff,cnxnFactory->st.sessionById.at(sessionId).password,16);
            response.passwd = passwd;
            response.sessionId = 5;
            response.protocolVersion = 3;
            struct oarchive *oa = create_buffer_oarchive();
            int32_t len = 36;
            oa->serialize_Int(oa,"",&len);
            serialize_ConnectResponse(oa,"connectResponse",&response);

            queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
            cout<<"发送connectResponse"<<endl;
        } else{
            response.timeOut = 0;
            struct buffer passwd;
            passwd.len = 16;
            passwd.buff = (char *)malloc(16);
            response.passwd = passwd;
            response.sessionId = 0;
            response.protocolVersion = 0;
            struct oarchive *oa = create_buffer_oarchive();
            int32_t len = 36;
            oa->serialize_Int(oa,"",&len);
            serialize_ConnectResponse(oa,"connectResponse",&response);

            queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
            cout<<"发送connectResponse"<<endl;
        }
    }else {
        sessionId = createCnxn(cnxnFactory,socketfd,sessionTimeout);
        cout<<"create session complete"<<endl;
        cout<<"sessionid = "<<sessionId<<endl;
        struct ConnectResponse response;
        response.timeOut = cnxnFactory->tick;
        struct buffer passwd;
        passwd.len = 16;
        passwd.buff = (char *)malloc(16);
        memcpy(passwd.buff,cnxnFactory->st.sessionById.at(sessionId).password,16);
        response.passwd = passwd;
        response.sessionId = 5;
        response.protocolVersion = 3;
        struct oarchive *oa = create_buffer_oarchive();
        int32_t len = 36;
        oa->serialize_Int(oa,"",&len);
        serialize_ConnectResponse(oa,"connectResponse",&response);

        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送connectResponse"<<endl;
    }
    return;
}


void processRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int64_t sessionId){
    serverCnxnFactory * cnxnFactory = &zServer->cnxnFactory;
    //deserialize header
    struct iarchive * ia = create_buffer_iarchive(buf,RequestHeaderLength);
    struct RequestHeader header;
    deserialize_RequestHeader(ia,"deserialize requestHeader",&header);
    //check session

    //cout<<"touch session "<<sessionId<<endl;
    touchSession(sessionId,zServer->cnxnFactory.tick,&cnxnFactory->st);

    if(header.type == ZOO_PING_OP){
        cout<<"ping"<<endl;
        struct ReplyHeader rh;
        rh.xid = -2;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        serialize_ReplyHeader(oa1,"ping",&rh);
        int32_t len = get_buffer_len(oa1);
        close_buffer_oarchive(&oa1,1);
        oa->serialize_Int(oa,"",&len);
        serialize_ReplyHeader(oa,"ping",&rh);

        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送ping response"<<endl;
    }else if(header.type == ZOO_CREATE_OP){
        cout<<"create op"<<endl;
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct CreateRequest createReq;
        deserialize_CreateRequest(ia,"deserialize create request",&createReq);

        //add node
        ZOO_ERRORS err = zoo_addNode(&zServer->zoo_dataTree,createReq.path,createReq.data,createReq.acl,
                                     sessionId,createReq.flags,getNextZxid(zServer), getTime(),cnxnFactory->cnxnList.at(sessionId).auth);
        //response
        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = err;

        struct oarchive *oa = create_buffer_oarchive();
        if(err == ZOK){
            string nodePath(createReq.path);
            string parentPath = nodePath.substr(0,nodePath.find_last_of('/'));

            triggerWatch(createReq.path,ZOO_CREATED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send);
            triggerWatch(const_cast<char*>(parentPath.c_str()),ZOO_CHILD_EVENT,&zServer->zoo_dataTree.childWatch,&zServer->to_send);
            struct oarchive *oa1 = create_buffer_oarchive();
            struct CreateResponse cr;
            cr.path = createReq.path;
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_CreateResponse(oa1,"create response",&cr);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_CreateResponse(oa,"create response",&cr);
        }else{
            struct oarchive *oa1 = create_buffer_oarchive();
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送createResponse"<<endl;

    }else if(header.type == ZOO_DELETE_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct DeleteRequest deleteReq;
        deserialize_DeleteRequest(ia,"deserialize delete request",&deleteReq);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();

        ZOO_ERRORS err = zoo_deleteNode(&zServer->zoo_dataTree,deleteReq.path,rh.zxid,cnxnFactory->cnxnList.at(sessionId).auth);
        rh.err = err;
        if(err == ZOK){
            triggerWatch(deleteReq.path,ZOO_DELETED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send);
        }
        serialize_ReplyHeader(oa1,"create op reply header",&rh);
        int32_t rLen = get_buffer_len(oa1);
        close_buffer_oarchive(&oa1,1);
        oa->serialize_Int(oa,"length",&rLen);
        serialize_ReplyHeader(oa,"create op reply header",&rh);


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送deleteResponse"<<endl;
    }else if(header.type == ZOO_EXISTS_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct ExistsRequest existReq;
        deserialize_ExistsRequest(ia,"deserialize exist request",&existReq);
        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        struct ExistsResponse er;

        ZOO_ERRORS err = zoo_isExist(&zServer->zoo_dataTree,existReq.path,&er);
        rh.err = err;
        if(err == ZOK){
            if(existReq.watch != 0){
                addWatch(existReq.path,&zServer->zoo_dataTree.dataWatch,sessionId);
            }
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_ExistsResponse(oa1,"get data response",&er);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_ExistsResponse(oa,"getdata response",&er);
        }else{
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送ExistsResponse"<<endl;


    }else if(header.type == ZOO_GETDATA_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct GetDataRequest getDataReq;
        deserialize_GetDataRequest(ia,"deserialize get data request",&getDataReq);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        struct GetDataResponse gdr;

        //get data
        ZOO_ERRORS err = zoo_getData(&zServer->zoo_dataTree,getDataReq.path,&gdr,cnxnFactory->cnxnList.at(sessionId).auth);
        rh.err = err;
        if(err == ZOK){
            if(getDataReq.watch != 0){
                addWatch(getDataReq.path,&zServer->zoo_dataTree.dataWatch,sessionId);
            }
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_GetDataResponse(oa1,"get data response",&gdr);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_GetDataResponse(oa,"getdata response",&gdr);
        }else{
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送getDataResponse"<<endl;

    }else if(header.type == ZOO_SETDATA_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct SetDataRequest setDataReq;
        deserialize_SetDataRequest(ia,"deserialize set data request",&setDataReq);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        struct SetDataResponse sdr;

        //get data
        ZOO_ERRORS err = zoo_setData(&zServer->zoo_dataTree,setDataReq.path,setDataReq.data,setDataReq.version,
                                     rh.zxid,getTime(),&sdr.stat,cnxnFactory->cnxnList.at(sessionId).auth);
        rh.err = err;
        if(err == ZOK){
            triggerWatch(setDataReq.path,ZOO_CHANGED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send);
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_SetDataResponse(oa1,"set data response",&sdr);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_SetDataResponse(oa,"setdata response",&sdr);
        }else{
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送setDataResponse"<<endl;

    }else if(header.type == ZOO_GETACL_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct GetACLRequest getAclReq;
        deserialize_GetACLRequest(ia,"deserialize get acl request",&getAclReq);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        struct GetACLResponse gar;

        //get acl
        ZOO_ERRORS err = zoo_getACL(&zServer->zoo_dataTree,getAclReq.path,&gar);
        rh.err = err;
        serialize_ReplyHeader(oa1,"create op reply header",&rh);
        serialize_GetACLResponse(oa1,"get acl response",&gar);
        int32_t rLen = get_buffer_len(oa1);
        close_buffer_oarchive(&oa1,1);
        oa->serialize_Int(oa,"length",&rLen);
        serialize_ReplyHeader(oa,"create op reply header",&rh);
        serialize_GetACLResponse(oa,"setdata response",&gar);

        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送getaclResponse"<<endl;

    }else if(header.type == ZOO_SETACL_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct SetACLRequest setAclReq;
        deserialize_SetACLRequest(ia,"deserialize set acl request",&setAclReq);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = 0;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        struct SetACLResponse sar;

        //set acl
        ZOO_ERRORS err = zoo_setACL(&zServer->zoo_dataTree,setAclReq.path,setAclReq.acl,setAclReq.version,cnxnFactory->cnxnList.at(sessionId).auth,&sar.stat);
        rh.err = err;
        if(err == ZOK){
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_SetACLResponse(oa1,"set acl response",&sar);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_SetACLResponse(oa,"set acl response",&sar);
        }else{
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送setAclResponse"<<endl;


    }else if(header.type == ZOO_GETCHILDREN_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct GetChildrenRequest getChildrenReq;
        deserialize_GetChildrenRequest(ia,"deserialize get children request",&getChildrenReq);

        struct GetChildrenResponse gcr;

        //get children
        ZOO_ERRORS err = zoo_getChildren(&zServer->zoo_dataTree,getChildrenReq.path,&gcr.children,cnxnFactory->cnxnList.at(sessionId).auth);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = getNextZxid(zServer);
        rh.err = err;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        if(err == ZOK){
            if(getChildrenReq.watch != 0){
                addWatch(getChildrenReq.path,&zServer->zoo_dataTree.childWatch,sessionId);
            }
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            serialize_GetChildrenResponse(oa1,"get children response",&gcr);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
            serialize_GetChildrenResponse(oa,"get children response",&gcr);
        }else{
            serialize_ReplyHeader(oa1,"create op reply header",&rh);
            int32_t rLen = get_buffer_len(oa1);
            close_buffer_oarchive(&oa1,1);
            oa->serialize_Int(oa,"length",&rLen);
            serialize_ReplyHeader(oa,"create op reply header",&rh);
        }


        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送get children Response"<<endl;

    }else if(header.type == ZOO_SETAUTH_OP){
        struct iarchive * ia = create_buffer_iarchive(buf+8,len-8);
        struct AuthPacket authPacket;
        deserialize_AuthPacket(ia,"deserialize addauth request",&authPacket);
        //add auth
        ZOO_ERRORS err = zoo_addAuth(&cnxnFactory->cnxnList.at(sessionId),authPacket);

        struct ReplyHeader rh;
        rh.xid = header.xid;
        rh.zxid = 0;
        rh.err = err;
        struct oarchive *oa = create_buffer_oarchive();
        struct oarchive *oa1 = create_buffer_oarchive();
        serialize_ReplyHeader(oa1,"ping",&rh);
        int32_t len = get_buffer_len(oa1);
        close_buffer_oarchive(&oa1,1);
        oa->serialize_Int(oa,"",&len);
        serialize_ReplyHeader(oa,"ping",&rh);
        queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
        cout<<"发送addauthResponse"<<endl;
    }else{
        cout<<"type=="<<header.type<<endl;
    }

}

int64_t getNextZxid(struct zookeeperServer * zServer){
    return ++zServer->zxid;
}

int64_t getTime(){
    time_t now;
    return time(&now);
}


int main(int argc, char * argv[]) {
    fprintf(stdout, "zServer starting on port %d\n", 2181);
    fprintf(stdout, "...waiting for connections\n");



    zookeeperServer * zServer = new zookeeperServer();
    zServer->Listener = 2181;
    zServer->cnxnFactory.tick = 10000;
    zServer->cnxnFactory.minSessionTimeout = 2*10000;
    zServer->cnxnFactory.maxSessionTimeout = 20*10000;
    map<int64_t ,int32_t > sessionWithTimeOut;
    zServer->cnxnFactory.st = initSessinTracker(0,10000,sessionWithTimeOut);
    zServer->zxid = 0;
    zServer->to_process.bufferLength = 0;




    thread acceptFromClients(listenOnPort,zServer);

    thread process(processPacket,zServer);

    thread sendResponseToClients(sendResponse,zServer);

    thread sessionTracker(processSessionTracker,&zServer->cnxnFactory.st);

    acceptFromClients.join();
    process.join();
    sendResponseToClients.join();
    sessionTracker.join();

    return 0;
}

void processPacket(zookeeperServer * zkServer){
    buffer_list_t * toProcess;
    while(1){
        toProcess = dequeue_buffer(&zkServer->to_process);
        if(!!toProcess){
            cout<<"process socket from "<<toProcess->sessionId<<endl;
            processRequest(toProcess->len,toProcess->buffer,zkServer,toProcess->sessionId);
        }else{
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }

}

void sendResponse(zookeeperServer * zkServer){
    buffer_list_t * toSend;
    while(1){
        toSend = dequeue_buffer(&zkServer->to_send);
        if(!!toSend){
            cout<<"send response to client"<<toSend->sessionId<<endl;
            write(zkServer->cnxnFactory.cnxnList.at(toSend->sessionId).socketfd,toSend->buffer,toSend->len);
        }else{
            this_thread::sleep_for(chrono::milliseconds(10));
        }
    }
}

void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
        perror("fcntl(sock,SETFL,opts)");
        exit(1);
    }
}
void listenOnPort(zookeeperServer * zkServer){
    int32_t i, listenfd, connfd, sockfd,epfd,nfds, portnumber;
    int64_t sessionId = 0;
    ssize_t readed;
    char line[1000];
    int32_t packetLength = 0;
    socklen_t clilen;
    portnumber = zkServer->Listener;

    map<int32_t ,int64_t >::iterator l_it;

    //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件

    struct epoll_event ev,events[maxConnections];
    //生成用于处理accept的epoll专用的文件描述符

    epfd=epoll_create(maxConnections+1);
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    //把socket设置为非阻塞方式
    //setnonblocking(listenfd);

    //设置与要处理的事件相关的文件描述符
    ev.data.fd=listenfd;
    //设置要处理的事件类型
    ev.events=EPOLLIN|EPOLLET;
    //注册epoll事件

    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char local_addr[]="127.0.0.1";
    inet_aton(local_addr,&(serveraddr.sin_addr));//htons(portnumber);

    serveraddr.sin_port=htons(portnumber);
    bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 100);
    for ( ; ; ) {
        //等待epoll事件的发生

        nfds=epoll_wait(epfd,events,maxConnections,-1);
        //处理所发生的所有事件
        for(i=0;i<nfds;++i)
        {
            if(events[i].data.fd==listenfd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。

            {
                connfd = accept(listenfd,(sockaddr *)&clientaddr, &clilen);
                if(connfd<0){
                    perror("connfd<0");
                    exit(1);
                }
                //setnonblocking(connfd);

                char *str = inet_ntoa(clientaddr.sin_addr);
                std::cout << "accept a connection from " << str << std::endl;


                //设置用于读操作的文件描述符
                ev.data.fd=connfd;
                //设置用于注测的读操作事件

                ev.events=EPOLLIN;
                //ev.events=EPOLLIN;

                //注册ev

                epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
            }
            else if(events[i].events&EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
            {

                if ( (sockfd = events[i].data.fd) < 0)
                    continue;
                if ( (readed = read(sockfd, &packetLength, sizeof(int))) < 0) {
                    if (errno == ECONNRESET) {
                        close(sockfd);
                        events[i].data.fd = -1;
                    } else
                        std::cout<<"readline error"<<std::endl;
                } else if (readed == 0) {
                    close(sockfd);
                    events[i].data.fd = -1;
                }
                packetLength = ntohl(packetLength);
                readed = read(sockfd, line, packetLength);
                if(readed == -1){
                    cout<<"readed == "<<readed<<endl;
                }else{
                    l_it=zkServer->cnxnFactory.cnxnIdBySocketfd.find(sockfd);
                    if(l_it==zkServer->cnxnFactory.cnxnIdBySocketfd.end()){
                        processConnectRequest(packetLength,line,zkServer,sockfd);
                    }else{
                        sessionId = l_it->second;
                        queue_buffer_bytes(&zkServer->to_process,line,packetLength,sessionId);
                    }
                }

            }
        }
    }
}



