//
// Created by haboa on 17-9-1.
//

#include "../include/watch_manager.h"
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

void triggerWatch(char * path,int32_t eventType,watchManager * manager,buffer_head_t * to_send){
    struct WatcherEvent event;
    event.path = path;
    event.type = eventType;
    event.state = ZOO_CONNECTED_STATE;
    string eventPath(event.path);
    set<watch> list;
    if(manager->watchTable.find(eventPath) != manager->watchTable.end()){
        list = manager->watchTable[eventPath];
        set<watch>::iterator it;
        for(it = list.begin();it != list.end();it++){
            processEvent(event,to_send,it->sessionId);
            set<string> paths = manager->watch2Path[it->sessionId];
            if(paths.size() != 0)
                manager->watch2Path[it->sessionId].erase(eventPath);
        }
        manager->watchTable.erase(eventPath);
    }else{
        return ;
    }
}

void processEvent(struct WatcherEvent event,buffer_head_t * to_send,int32_t sessionId){
    struct ReplyHeader rh;
    rh.xid = -1;
    rh.zxid = -1L;
    rh.err = 0;
    struct oarchive *oa = create_buffer_oarchive();
    struct oarchive *oa1 = create_buffer_oarchive();
    serialize_ReplyHeader(oa1,"event op reply header",&rh);
    serialize_WatcherEvent(oa1,"event response",&event);
    int32_t rLen = get_buffer_len(oa1);
    close_buffer_oarchive(&oa1,1);
    oa->serialize_Int(oa,"length",&rLen);
    serialize_ReplyHeader(oa,"event op reply header",&rh);
    serialize_WatcherEvent(oa,"event response",&event);

    queue_buffer_bytes(to_send,get_buffer(oa),get_buffer_len(oa),sessionId);
    cout<<"发送event Response"<<endl;
}
