//
// Created by haboa on 17-9-1.
//

#include "../include/watch_manager.h"
#include "../include/serverCnxn.h"
#include <iostream>

bool operator < (const watch &a,const watch &b){
    return a.sessionId  < b.sessionId;
}
bool operator == (const watch &a,const watch &b){
    return a.sessionId  == b.sessionId;
}

void addWatch(char * path,watchManager * manager,int64_t sessionId){
    watch w;
    w.sessionId = sessionId;
    string watchPath(path);
    if(manager->watchTable.find(watchPath) != manager->watchTable.end()){
        manager->watchTable[watchPath].insert(w);
    }else{
        set<watch> watchList;
        watchList.insert(w);
        manager->watchTable[watchPath] = watchList;
    }

    if(manager->watch2Path.find(sessionId) != manager->watch2Path.end()){
        manager->watch2Path[sessionId].insert(watchPath);
    }else{
        set<string> pathList;
        pathList.insert(watchPath);
        manager->watch2Path[sessionId] = pathList;
    }

}

void triggerWatch(char * path,int32_t eventType,watchManager * manager,buffer_head_t * to_send,serverCnxnFactory * cnxnFactory){
    struct WatcherEvent event;
    event.path = path;
    event.type = eventType;
    event.state = ZOO_CONNECTED_STATE;
    string eventPath(event.path);
    set<watch> list;
    map<int64_t ,Session>::iterator sessionIt;
    if(manager->watchTable.find(eventPath) != manager->watchTable.end()){
        list = manager->watchTable[eventPath];
        set<watch>::iterator it;
        for(it = list.begin();it != list.end();it++){
            //send watch triggerd to the client
            sessionIt = cnxnFactory->st.sessionById.find(it->sessionId);
            if(sessionIt != cnxnFactory->st.sessionById.end()){
                if(sessionIt->second.owner == cnxnFactory->st.serverId){
                    processEvent(event,to_send,it->sessionId,cnxnFactory);
                    std::cout<<"sending event type = "<<event.type<<":event path = "<<event.path<<": sessionId = "<<it->sessionId<<endl;
                }
            }
            set<string> paths = manager->watch2Path[it->sessionId];
            if(paths.size() != 0)
                manager->watch2Path[it->sessionId].erase(eventPath);
        }
        manager->watchTable.erase(eventPath);
    }else{
        return ;
    }
}

void deleteWatch(int64_t sessionId,watchManager * manager){
    unordered_map<int64_t ,set<string>>::iterator watch2PathIterator;
    watch2PathIterator = manager->watch2Path.find(sessionId);
    if(watch2PathIterator == manager->watch2Path.end()){
        return ;
    }
    set<string>::iterator iter=watch2PathIterator->second.begin();
    string path = "";
    watch w;
    w.sessionId = sessionId;
    while(iter!=watch2PathIterator->second.end())
    {
        path = *iter;
        manager->watchTable.at(path).erase(w);
        ++iter;
    }
    manager->watch2Path.erase(sessionId);

}

void processEvent(struct WatcherEvent event,buffer_head_t * to_send,int64_t sessionId,serverCnxnFactory * cnxnFactory){
    struct ReplyHeader rh;
    rh.xid = -1;
    rh.zxid = -1L;
    rh.err = 0;
    int32_t len = 0;
    struct oarchive *oa = create_buffer_oarchive();
    oa->serialize_Int(oa,"",&len);
    serialize_ReplyHeader(oa,"event op reply header",&rh);
    serialize_WatcherEvent(oa,"event response",&event);
    int32_t fd = getFd(sessionId,cnxnFactory);
    if(fd != -1){
        queue_buffer_reply_bytes(to_send,get_buffer(oa),get_buffer_len(oa),sessionId,fd);
        cout<<"send event Response"<<endl;
    }
    close_buffer_oarchive(&oa,1);
}
