//
// Created by forec on 17-5-8.
//

#include "../include/zookeeperServer.h"
#include "../include/serverCnxn.h"
#include <fcntl.h>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>


void processConnectRequest(int32_t len,char * buf,struct zookeeperServer * zServer,int32_t socketfd,MsgQueue * pro_mq){
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
        map<int64_t ,Session>::iterator it;
        it = cnxnFactory->st.sessionById.find(sessionId);
        struct ConnectResponse response;
        if(it == cnxnFactory->st.sessionById.end()){//session timeout . need to recreate a new session
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
            int32_t fd = getFd(sessionId,cnxnFactory);
            if(fd != -1){
                queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                cout<<"发送connectResponse"<<endl;
            }
        }else{
            int64_t valid = createSession(&zServer->cnxnFactory.st,cr.timeOut);
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
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"发送connectResponse"<<endl;
                }
            }
        }

    }else {
        sessionId = createCnxn(cnxnFactory,socketfd,sessionTimeout);
        cout<<"create session complete"<<endl;
        cout<<"sessionid = "<<sessionId<<endl;
        //init create session request
        Session session = cnxnFactory->st.sessionById.at(sessionId);
        struct RequestHeader rh;
        rh.type = ZOO_CREATESESSION_OP;
        rh.xid = 100;
        struct CreateSessionRequest createSessionrsq;
        createSessionrsq.sessionId = sessionId;
        createSessionrsq.sessionTimeout = session.sessionTimeout;
        createSessionrsq.passwd.len = 16;
        memcpy(createSessionrsq.passwd.buff,session.password,16);
        struct oarchive * cspOa = create_buffer_oarchive();
        serialize_RequestHeader(cspOa,"",&rh);
        serialize_CreateSessionRequest(cspOa,"serialize create session request ",&createSessionrsq);
        int packetLen = get_buffer_len(cspOa);
        char * buff = (char *)malloc(packetLen);
        memcpy(buff,get_buffer(cspOa),packetLen);
        //init tk_command
        tk_command * command = new tk_command();
        command->opcode = CREATESESSION;
        command->key = "";
        command->owner = zServer->serverId;
        command->sessionId = sessionId;
        command->valSize = packetLen;
        command->val = buff;
        //init propose
        Propose *propose = new Propose();
        propose->Command = *command;
        pro_mq->put(&propose);
        //send response to client
        struct ConnectResponse response;
        response.timeOut = cnxnFactory->tick;
        struct buffer passwd;
        passwd.len = 16;
        passwd.buff = (char *)malloc(16);
        memcpy(passwd.buff,cnxnFactory->st.sessionById.at(sessionId).password,16);
        response.passwd = passwd;
        response.sessionId = sessionId;
        response.protocolVersion = 3;
        struct oarchive *oa = create_buffer_oarchive();
        int32_t len = 36;
        oa->serialize_Int(oa,"",&len);
        serialize_ConnectResponse(oa,"connectResponse",&response);
        int32_t fd = getFd(sessionId,cnxnFactory);
        if(fd != -1){
            cout<<"fd="<<fd<<"socketfd="<<socketfd<<endl;
            queue_buffer_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
            cout<<"发送connectResponse"<<endl;
        }
        close_buffer_oarchive(&oa,1);
        close_buffer_oarchive(&cspOa,1);
    }
    return;
}

void pre_processRequest(struct zookeeperServer * zServer,buffer_list_t * toProcess,MsgQueue * pro_mq){
    int32_t len = toProcess->len;
    int64_t sessionId = toProcess->sessionId;
    serverCnxnFactory * cnxnFactory = &zServer->cnxnFactory;
    Propose * propose = new Propose();
    //deserialize header
    struct iarchive * ia;
    ia = create_buffer_iarchive(toProcess->buffer,RequestHeaderLength);
    struct RequestHeader header;
    deserialize_RequestHeader(ia,"deserialize requestHeader",&header);
    //check session
    touchSession(sessionId,zServer->cnxnFactory.tick,&cnxnFactory->st);
    switch(header.type){
        case ZOO_CREATE_OP: {
            cout<<"pre create op"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct CreateRequest createReq;
            deserialize_CreateRequest(ia,"deserialize create request",&createReq);
            //init tk_command
            tk_command * command = new tk_command();
            command->owner = zServer->serverId;
            command->key = createReq.path;
            cout<<"pre create path="<<command->key<<endl;
            cout<<"pre create flag="<<createReq.flags<<endl;
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = (char *)malloc(len);
            memcpy(command->val,toProcess->buffer,len);
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }

        case ZOO_DELETE_OP: {
            cout<<"pre delete op"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct DeleteRequest deleteReq;
            deserialize_DeleteRequest(ia,"deserialize delete request",&deleteReq);
            tk_command * command = new tk_command();
            //init tk_command
            command->owner = zServer->serverId;
            command->key = deleteReq.path;
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = toProcess->buffer;
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }
        case ZOO_SETDATA_OP: {
            cout<<"pre set data"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct SetDataRequest setDataReq;
            deserialize_SetDataRequest(ia,"deserialize set data request",&setDataReq);
            tk_command * command = new tk_command();
            //init tk_command
            command->owner = zServer->serverId;
            command->key = setDataReq.path;
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = toProcess->buffer;
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }
        case ZOO_SETACL_OP: {
            cout<<"pre set acl"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct SetACLRequest setAclReq;
            deserialize_SetACLRequest(ia,"deserialize set acl request",&setAclReq);
            tk_command * command = new tk_command();
            //init tk_command
            command->owner = zServer->serverId;
            command->key = setAclReq.path;
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = toProcess->buffer;
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }
        case ZOO_SETAUTH_OP: {
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct AuthPacket authPacket;
            deserialize_AuthPacket(ia,"deserialize addauth request",&authPacket);
            tk_command * command = new tk_command();
            //init tk_command
            command->owner = zServer->serverId;
            command->key = authPacket.scheme;
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = toProcess->buffer;
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }
        case ZOO_CLOSE_OP:{
            //send close sessin to all other server
            //init tk_command
            tk_command * command = new tk_command();
            command->owner = zServer->serverId;
            command->key = "";
            command->sessionId = sessionId;
            command->valSize = len;
            command->val = (char *)malloc(len);
            command->opcode = CLOSESESSION;
            memcpy(command->val,toProcess->buffer,len);
            //init propose
            propose->Command = *command;
            pro_mq->put(&propose);
            break;
        }
        case ZOO_PING_OP:
        case ZOO_EXISTS_OP:
        case ZOO_GETCHILDREN_OP:
        case ZOO_GETDATA_OP:
        case ZOO_GETACL_OP:
        default :
            queue_buffer(&zServer->to_process,toProcess,0);
    }
    close_buffer_iarchive(&ia);
}

void processRequest(struct zookeeperServer * zServer,buffer_list_t * toProcess){
    bool isOwnerThis = false;
    cout<<"packet owner = "<<toProcess->ownerOrfd<<"||serverid = "<<zServer->serverId<<"||sessionId="<<toProcess->sessionId<<endl;
    if(toProcess->ownerOrfd == zServer->serverId){
        isOwnerThis = true;
    }
    int32_t len = toProcess->len;
    int64_t sessionId = toProcess->sessionId;
    serverCnxnFactory * cnxnFactory = &zServer->cnxnFactory;
    //deserialize header
    struct iarchive * ia;
    ia = create_buffer_iarchive(toProcess->buffer,RequestHeaderLength);
    struct RequestHeader header;
    deserialize_RequestHeader(ia,"deserialize requestHeader",&header);

    ZOO_ERRORS err;
    int32_t rlen = 0;
    struct oarchive * oa = create_buffer_oarchive();
    struct ReplyHeader rh;
    switch(header.type){
        case ZOO_CREATESESSION_OP:{
            cout<<"zoo create session"<<endl;
            cout<<"serverId"<<zServer->serverId<<endl;
            if(toProcess->ownerOrfd == zServer->serverId)
                break;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            cout<<"deserialize createsession request"<<endl;
            struct CreateSessionRequest createSessionreq;
            deserialize_CreateSessionRequest(ia,"deserialize create session request",&createSessionreq);
            //create session
            Session session;
            session.sessionId = sessionId;
            session.owner = toProcess->ownerOrfd;
            session.sessionTimeout = createSessionreq.sessionTimeout;
            cout<<"memory copy"<<endl;
            strncpy(session.password,createSessionreq.passwd.buff,15);
            cout<<"create session complete"<<sessionId<<endl;
            session.isClosing = false;
            createCnxn(&zServer->cnxnFactory,session);
            map<int64_t ,serverCnxn>::iterator it;
            cout<<"session id"<<session.sessionId<<endl;
            it = cnxnFactory->cnxnList.find(sessionId);
            if(it == cnxnFactory->cnxnList.end()){
                cout<<"just insert but could not find session"<<endl;
            }
            break;
        }
        case ZOO_CLOSE_OP:{
            cout<<"close session op"<<endl;
            //delete session & cnxn
            removeSession(sessionId,zServer);
            //send response to client
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = ZCLOSING;
                oa->serialize_Int(oa,"",&rlen);
                serialize_ReplyHeader(oa,"close",&rh);
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send close response to client"<<endl;
                }
            }
            cout<<"close session complete"<<sessionId<<endl;
            break;
        }
        case ZOO_PING_OP:{
            cout<<"ping"<<endl;
            rh.xid = -2;
            rh.zxid = getNextZxid(zServer);
            rh.err = 0;
            oa->serialize_Int(oa,"",&rlen);
            serialize_ReplyHeader(oa,"ping",&rh);
            int32_t fd = getFd(sessionId,cnxnFactory);
            if(fd != -1){
                queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                //cout<<"send ping response to client"<<endl;
            }
            break;}
        case ZOO_CREATE_OP:{
            cout<<"create op"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct CreateRequest createReq;
            deserialize_CreateRequest(ia,"deserialize create request",&createReq);

            //add node
            map<int64_t ,serverCnxn>::iterator it;
            it = cnxnFactory->cnxnList.find(sessionId);
            if(it == cnxnFactory->cnxnList.end()){
                cout<<"could not find session "<<endl;
            } else
                cout<<"session found"<<endl;
            cout<<createReq.flags<<"flags"<<endl;
            string bu = createReq.data.buff;
            cout<<"data"<<bu<<endl;
            string nodepath = createReq.path;
            cout<<"nodepath="<<nodepath<<endl;
            err = zoo_addNode(&zServer->zoo_dataTree,createReq.path,createReq.data,createReq.acl,
                                         sessionId,createReq.flags,getNextZxid(zServer), getTime(),cnxnFactory->cnxnList.at(sessionId).auth);
            //trigger watch
            cout<<"create op result err = "<<ZOK<<"triggering watch"<<endl;
            if(err == ZOK){
                string nodePath(createReq.path);
                string parentPath = nodePath.substr(0,nodePath.find_last_of('/'));
                triggerWatch(createReq.path,ZOO_CREATED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send,&zServer->cnxnFactory);
                triggerWatch(const_cast<char*>(parentPath.c_str()),ZOO_CHILD_EVENT,&zServer->zoo_dataTree.childWatch,&zServer->to_send,&zServer->cnxnFactory);
            }
            //response
            cout<<"trigger watch done! response to client"<<endl;
            if(isOwnerThis){
                cout<<"this is the owner"<<endl;
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    struct CreateResponse cr;
                    cr.path = createReq.path;
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"create op reply header",&rh);
                    serialize_CreateResponse(oa,"create response",&cr);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"create op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send create Response"<<endl;
                }
            }
            break;}
        case ZOO_DELETE_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct DeleteRequest deleteReq;
            deserialize_DeleteRequest(ia,"deserialize delete request",&deleteReq);
            //delete node
            err = zoo_deleteNode(&zServer->zoo_dataTree,deleteReq.path,rh.zxid,cnxnFactory->cnxnList.at(sessionId).auth);
            //trigger watch
            if(err == ZOK){
                triggerWatch(deleteReq.path,ZOO_DELETED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send,&zServer->cnxnFactory);
            }
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                oa->serialize_Int(oa,"length",&rlen);
                serialize_ReplyHeader(oa,"delete  op reply header",&rh);
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send delete Response"<<endl;
                }
            }
            break;}
        case ZOO_EXISTS_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct ExistsRequest existReq;
            deserialize_ExistsRequest(ia,"deserialize exist request",&existReq);
            //is exist
            struct ExistsResponse er;
            err = zoo_isExist(&zServer->zoo_dataTree,existReq.path,&er);
            //add watch
            if(err == ZOK){
                if(existReq.watch != 0){
                    addWatch(existReq.path,&zServer->zoo_dataTree.dataWatch,sessionId);
                }
            }
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"is exist reply header",&rh);
                    serialize_ExistsResponse(oa,"is exist response",&er);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"is exist op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send exist Response"<<endl;
                }
            }
            break;}
        case ZOO_GETDATA_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct GetDataRequest getDataReq;
            deserialize_GetDataRequest(ia,"deserialize get data request",&getDataReq);
            //get data
            struct GetDataResponse gdr;
            err = zoo_getData(&zServer->zoo_dataTree,getDataReq.path,&gdr,cnxnFactory->cnxnList.at(sessionId).auth);
            //trigger watch
            if(err == ZOK){
                if(getDataReq.watch != 0){
                    addWatch(getDataReq.path,&zServer->zoo_dataTree.dataWatch,sessionId);
                }
            }
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"get data op reply header",&rh);
                    serialize_GetDataResponse(oa,"get data response",&gdr);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"get data op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send get data Response"<<endl;
                }
            }
            break;}
        case ZOO_SETDATA_OP:{
            cout<<"set data"<<endl;
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct SetDataRequest setDataReq;
            deserialize_SetDataRequest(ia,"deserialize set data request",&setDataReq);
            //set data
            struct SetDataResponse sdr;
            err = zoo_setData(&zServer->zoo_dataTree,setDataReq.path,setDataReq.data,setDataReq.version,
                              rh.zxid,getTime(),&sdr.stat,cnxnFactory->cnxnList.at(sessionId).auth);
            //trigger watch
            if(err == ZOK){
                cout<<"trigger set data watch"<<endl;
                triggerWatch(setDataReq.path,ZOO_CHANGED_EVENT,&zServer->zoo_dataTree.dataWatch,&zServer->to_send,&zServer->cnxnFactory);
            }
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"set data op reply header",&rh);
                    serialize_SetDataResponse(oa,"set data response",&sdr);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"set data op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send set data Response"<<endl;
                }
            }
            break;}
        case ZOO_GETACL_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct GetACLRequest getAclReq;
            deserialize_GetACLRequest(ia,"deserialize get acl request",&getAclReq);
            //get acl
            struct GetACLResponse gar;
            err = zoo_getACL(&zServer->zoo_dataTree,getAclReq.path,&gar);
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                oa->serialize_Int(oa,"length",&rlen);
                serialize_ReplyHeader(oa,"get acl op reply header",&rh);
                serialize_GetACLResponse(oa,"get acl response",&gar);
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send get acl Response"<<endl;
                }
            }
            break;}
        case ZOO_SETACL_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct SetACLRequest setAclReq;
            deserialize_SetACLRequest(ia,"deserialize set acl request",&setAclReq);
            //set acl
            struct SetACLResponse sar;
            err = zoo_setACL(&zServer->zoo_dataTree,setAclReq.path,setAclReq.acl,setAclReq.version,cnxnFactory->cnxnList.at(sessionId).auth,&sar.stat);
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"set acl op reply header",&rh);
                    serialize_SetACLResponse(oa,"set acl response",&sar);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"set acl op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send set acl Response"<<endl;
                }
            }
            break;}
        case ZOO_GETCHILDREN_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct GetChildrenRequest getChildrenReq;
            deserialize_GetChildrenRequest(ia,"deserialize get children request",&getChildrenReq);
            //get children
            struct GetChildrenResponse gcr;
            err = zoo_getChildren(&zServer->zoo_dataTree,getChildrenReq.path,&gcr.children,cnxnFactory->cnxnList.at(sessionId).auth);
            //add watch
            if(err == ZOK){
                if(getChildrenReq.watch != 0){
                    addWatch(getChildrenReq.path,&zServer->zoo_dataTree.childWatch,sessionId);
                }
            }
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = getNextZxid(zServer);
                rh.err = err;
                if(err == ZOK){
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"get children op reply header",&rh);
                    serialize_GetChildrenResponse(oa,"get children response",&gcr);
                }else{
                    oa->serialize_Int(oa,"length",&rlen);
                    serialize_ReplyHeader(oa,"get children op reply header",&rh);
                }
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send get children Response"<<endl;
                }
            }
            break;}
        case ZOO_SETAUTH_OP:{
            ia = create_buffer_iarchive(toProcess->buffer+8,len-8);
            struct AuthPacket authPacket;
            deserialize_AuthPacket(ia,"deserialize addauth request",&authPacket);
            //add auth
            err = zoo_addAuth(&cnxnFactory->cnxnList.at(sessionId),authPacket);
            //response
            if(isOwnerThis){
                rh.xid = header.xid;
                rh.zxid = 0;
                rh.err = err;
                oa->serialize_Int(oa,"",&rlen);
                serialize_ReplyHeader(oa,"ping",&rh);
                int32_t fd = getFd(sessionId,cnxnFactory);
                if(fd != -1){
                    queue_buffer_reply_bytes(&zServer->to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
                    cout<<"send set auth Response"<<endl;
                }
            }
            break;}
        default :
            cout<<"header type = "<<header.type<<endl;
            cout<<"header xid = "<<header.xid<<endl;
    }
    close_buffer_oarchive(&oa,1);
    close_buffer_iarchive(&ia);

}

int64_t getNextZxid(struct zookeeperServer * zServer){
    return ++zServer->zxid;
}

int64_t getTime(){
    time_t now;
    return time(&now);
}

bool initZookeeperServer(zookeeperServer * zServer,int32_t listenPort,int32_t serverId){
    cout<<"initing zookeeperServer with listen port"<<listenPort<<endl;
    zServer->Listener = listenPort;
    zServer->cnxnFactory.tick = 10000;
    zServer->cnxnFactory.minSessionTimeout = 2*10000;
    zServer->cnxnFactory.maxSessionTimeout = 20*10000;
    map<int64_t ,int32_t > sessionWithTimeOut;
    zServer->cnxnFactory.st = initSessinTracker(serverId,10000,sessionWithTimeOut);
    int64_t zxid = serverId;
    zServer->zxid = zxid << 32;
    zServer->to_process.bufferLength = 0;
    zServer->serverId = serverId;
    zServer->zoo_dataTree.root.path = "/";
    string fileName = "data";
    int fd = open(fileName.c_str(),O_RDONLY);
    if(fd < 0){
        cout<<"there is no data file"<<endl;
    }else{
        readDataTreeFromFile(&zServer->zoo_dataTree,fileName,fd);
    }
    return true;
}

void pre_processPacket(zookeeperServer * zkServer,MsgQueue * pro_mq){
    buffer_list_t * toProcess;
    while(1){
        toProcess = dequeue_buffer(&zkServer->pre_process);
        if(!!toProcess){
            //cout<<"pre_process socket from "<<toProcess->sessionId<<endl;
            pre_processRequest(zkServer,toProcess,pro_mq);
        }
    }
}

void processPacket(zookeeperServer * zkServer){
    buffer_list_t * toProcess;
    while(1){
        toProcess = dequeue_buffer(&zkServer->to_process);
        if(!!toProcess){
            //cout<<"process socket from "<<toProcess->sessionId<<endl;
            processRequest(zkServer,toProcess);
        }
    }
}


void sendResponse(zookeeperServer * zkServer){
    buffer_list_t * toSend;
    while(1){
        toSend = dequeue_buffer(&zkServer->to_send);
        if(!!toSend){
            cout<<"send response to client"<<toSend->sessionId<<"socketfd="<<toSend->ownerOrfd<<endl;
            write(toSend->ownerOrfd,toSend->buffer,toSend->len);
        }
    }
}


void removeSession(int64_t sessionId,zookeeperServer * zkServer){
    //session is connected to this server?
    map<int64_t ,Session>::iterator it;
    serverCnxnFactory * cnxnFactory = &zkServer->cnxnFactory;
    it = cnxnFactory->st.sessionById.find(sessionId);
    if(it == cnxnFactory->st.sessionById.end()){
        return;
    }
    //remove session from session tracker
    lockSessionTracker(&cnxnFactory->st);
    cnxnFactory->st.sessionById.erase(sessionId);
    unLockSessionTracker(&cnxnFactory->st);
    cout<<"remove session complete"<<endl;
    //remove ephemerals
    dataTree * dt = &zkServer->zoo_dataTree;
    unordered_map<int64_t ,set<string>>::iterator ephIt;
    ephIt = dt->ephemerals.find(sessionId);
    if(ephIt != dt->ephemerals.end()){
        dt->ephemerals.erase(sessionId);
    }
    cout<<"remove ephemerals complete"<<endl;
    //remove watches
    deleteWatch(sessionId,&zkServer->zoo_dataTree.dataWatch);
    deleteWatch(sessionId,&zkServer->zoo_dataTree.childWatch);
    cout<<"delete watched complete"<<endl;
    //remove session from cnxn factory
    map<int64_t ,serverCnxn>::iterator cnxnIt;
    cnxnIt = cnxnFactory->cnxnList.find(sessionId);
    if(cnxnIt == cnxnFactory->cnxnList.end()){
        return ;
    }else{
        int32_t fd = cnxnIt->second.socketfd;
        cnxnFactory->cnxnList.erase(sessionId);
        map<int32_t ,int64_t >::iterator fdIt;
        fdIt = cnxnFactory->cnxnIdBySocketfd.find(fd);
        if(fdIt != cnxnFactory->cnxnIdBySocketfd.end())
            cnxnFactory->cnxnIdBySocketfd.erase(fd);
    }
    cout<<"remove cnxn complete"<<endl;
    return ;
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
void listenOnPort(zookeeperServer * zkServer,MsgQueue * pro_mq){
    int32_t i, listenfd, connfd, sockfd,epfd,nfds, portnumber;
    int64_t sessionId = 0;
    ssize_t readed;
    char line[1000];
    int32_t packetLength = 0;
    socklen_t clilen;
    portnumber = zkServer->Listener;

    map<int32_t ,int64_t >::iterator l_it;

    //声明epoll_event结构体的变量,ev用于注册事件,数组用于回传要处理的事件

    struct epoll_event ev;
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
    serveraddr.sin_addr.s_addr = htons(INADDR_ANY);
    char local_addr[]="127.0.0.1";
    //inet_aton(local_addr,&(serveraddr.sin_addr));//htons(portnumber);

    serveraddr.sin_port=htons(portnumber);
    bind(listenfd,(sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 100);
    for ( ; ; ) {
        //等待epoll事件的发生

        nfds=epoll_wait(epfd,zkServer->events,maxConnections,-1);
        //处理所发生的所有事件
        for(i=0;i<nfds;++i)
        {
            if(zkServer->events[i].data.fd==listenfd)//如果新监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。

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
            else if(zkServer->events[i].events&EPOLLIN)//如果是已经连接的用户，并且收到数据，那么进行读入。
            {

                if ( (sockfd = zkServer->events[i].data.fd) < 0)
                    continue;
                if ( (readed = read(sockfd, &packetLength, sizeof(int))) < 0) {
                    if (errno == ECONNRESET) {
                        close(sockfd);
                        zkServer->events[i].data.fd = -1;
                    } else
                        std::cout<<"readline error"<<std::endl;
                } else if (readed == 0) {
                    close(sockfd);
                    zkServer->events[i].data.fd = -1;
                }
                packetLength = ntohl(packetLength);
                readed = read(sockfd, line, packetLength);
                if(readed == -1){
                    cout<<"readed == "<<readed<<endl;
                }else{
                    l_it=zkServer->cnxnFactory.cnxnIdBySocketfd.find(sockfd);
                    if(l_it==zkServer->cnxnFactory.cnxnIdBySocketfd.end()){
                        processConnectRequest(packetLength,line,zkServer,sockfd,pro_mq);
                    }else{
                        sessionId = l_it->second;
                        queue_buffer_bytes(&zkServer->pre_process,line,packetLength,sessionId,zkServer->serverId);
                    }
                }

            }
        }
    }
}



