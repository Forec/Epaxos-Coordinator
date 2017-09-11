//
// Created by haoba on 17-7-21.
//

#include "../include/serverCnxn.h"
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>
using namespace std;



int32_t passwordLen = 16;
int64_t superSecret = 0XB3415C00L;

int64_t getCurrentTimeMillion() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

ZOO_ERRORS zoo_addAuth(serverCnxn * cnxn,struct AuthPacket authPacket){
    struct Id id;
    id.id = (char *)malloc(authPacket.auth.len+1);
    memcpy(id.id,authPacket.auth.buff,authPacket.auth.len);
    id.id[authPacket.auth.len] = '\0';
    id.scheme = authPacket.scheme;
    cnxn->auth.push_back(id);
    return ZOK;
}


int64_t createCnxn(serverCnxnFactory * cnxnFactory,int32_t socketfd,int32_t timeout){
    int64_t sessionId = createSession(&cnxnFactory->st,timeout);
    serverCnxn cnxn;
    cnxn.sessionId = sessionId;
    cnxn.socketfd = socketfd;
    cnxnFactory->cnxnList.insert(make_pair(sessionId,cnxn));
    cnxnFactory->cnxnIdBySocketfd.insert(make_pair(socketfd,sessionId));
    cout<<"sessionId = "<<sessionId<<endl;
    return sessionId;
}

int64_t initializeNextSession(long id) {
    int64_t nextSid = 0;
    struct timeval tv;                //获取一个时间结构
    gettimeofday(&tv, NULL);   //获取当前时间
    int64_t time = tv.tv_sec;
    time *=1000;
    time +=tv.tv_usec/1000;
    nextSid = (time << 24) >> 8;
    nextSid =  nextSid | (id <<56);
    return nextSid;
}


int64_t createSessionId(sessionTracker* st,int32_t timeout){

    return st->nextSessionId++;
}

int64_t createSession(sessionTracker * st,int32_t timeout){
    std::cout<<"create a session"<<std::endl;
    int64_t sessionId = createSessionId(st,timeout);
    addSession(st,sessionId,timeout);
    cout<<"add session complete"<<endl;
    return sessionId;
}

bool addSession(sessionTracker * st,int64_t sessionId,int32_t timeout){
    st->sessionWithTimeOut.insert(make_pair(sessionId,timeout));
    map<int64_t ,Session>::iterator lt;
    lt = st->sessionById.find(sessionId);
    if(lt == st->sessionById.end()){
        //create a new session
        cout<<"create a new session,sessionId="<<sessionId<<endl;
        Session session;
        getPassword(session.password,sessionId);
        session.sessionId = sessionId;
        session.sessionTimeout = timeout;
        session.tickTime = 0;
        session.isClosing = false;
        session.owner = st->serverId;
        st->sessionById.insert(make_pair(sessionId,session));
    }else{
        lt->second.isClosing = true;
        lt->second.owner = st->serverId;
    }
    cout<<"ready to touch session"<<endl;
    return touchSession(sessionId,timeout,st);
}


int64_t roundToInterval(int64_t time,sessionTracker * st) {
    // We give a one interval grace period
    return (time / st->expirationInterval + 1) * st->expirationInterval;
}

bool touchSession(int64_t sessionId,int32_t timeout,sessionTracker * st){
    lockSessionTracker(st);
    map<int64_t ,Session>::iterator lt;
    lt = st->sessionById.find(sessionId);
    if(lt == st->sessionById.end()){
        unLockSessionTracker(st);
        return false;
    }else{
        if(lt->second.isClosing){
            cout<<"session is closed"<<endl;
            unLockSessionTracker(st);
            return false;
        }
        int64_t expireTime = roundToInterval(getCurrentTimeMillion() + timeout,st);
        if(lt->second.tickTime >= expireTime){
            // Nothing needs to be done
            unLockSessionTracker(st);
            return true;
        }
        set<Session> sessions;
        map<int64_t ,set<Session>>::iterator it;
        it = st->sessionSets.find(lt->second.tickTime);
        if(it != st->sessionSets.end()){
            cout<<"erase expire time"<<lt->second.tickTime<<endl;
            st->sessionSets.at(lt->second.tickTime).erase(lt->second);
        }
        lt->second.tickTime = expireTime;
        it = st->sessionSets.find(lt->second.tickTime);
        if(it == st->sessionSets.end()){
            sessions.insert(lt->second);
            st->sessionSets.insert(make_pair(lt->second.tickTime,sessions));
        }else{
            it->second.insert(lt->second);
        }
    }
    unLockSessionTracker(st);
    return true;
}

void getPassword(char *str,int64_t sessionId)
{
    srand(sessionId ^ superSecret);
    int i;
    for(i=0;i<passwordLen;++i)
    {
        switch((rand()%3))
        {
            case 1:
                str[i]='A'+rand()%26;
                break;
            case 2:
                str[i]='a'+rand()%26;
                break;
            default:
                str[i]='0'+rand()%10;
                break;
        }
    }
}

void processSessionTracker(sessionTracker * st){
    int64_t currentTime = 0;
    while(1){
        currentTime = getCurrentTimeMillion();
        if (st->nextExpirationTime > currentTime) {
            usleep((st->nextExpirationTime - currentTime)*1000);
            continue;
        }
        set<Session> sessions;
        map<int64_t ,set<Session>>::iterator it;
        lockSessionTracker(st);
        it = st->sessionSets.find(st->nextExpirationTime);
        if(it != st->sessionSets.end()){
            sessions = it->second;
            st->sessionSets.erase(st->nextExpirationTime);
            set<Session>::iterator sit;
            for(sit = sessions.begin();sit != sessions.end();sit++){
                setSessionClosing(sit->sessionId,st);
                //send close session to client
                cout<<"session expired "<<st->nextExpirationTime<<endl;
                //queue_buffer_bytes()
            }
        }
        st->nextExpirationTime += st->expirationInterval;
        unLockSessionTracker(st);
    }
}

void setSessionClosing(int64_t sessionId,sessionTracker * st){
    map<int64_t ,Session>::iterator it;
    it = st->sessionById.find(sessionId);
    if(it != st->sessionById.end()){
        it->second.isClosing = true;
    }
}

bool operator < (const Session & a,const Session &b){
    return a.sessionId < b.sessionId;
}


sessionTracker initSessinTracker(int64_t nextSessionId,int32_t tickTime,map<int64_t ,int32_t > sessionWithTimeOut){
    sessionTracker st;
    st.nextSessionId = initializeNextSession(nextSessionId);
    st.expirationInterval = tickTime;
    st.nextExpirationTime = roundToInterval(getCurrentTimeMillion(),&st);
    st.lock = PTHREAD_MUTEX_INITIALIZER;
    return st;
}

int32_t lockSessionTracker(sessionTracker * st){
    return pthread_mutex_lock(&st->lock);
}

int32_t unLockSessionTracker(sessionTracker * st){
    return pthread_mutex_unlock(&st->lock);
}


ZOO_ERRORS checkSession(int64_t sessionId,sessionTracker * st,int64_t serverId){
    map<int64_t ,Session>::iterator it;
    it = st->sessionById.find(sessionId);
    if(it == st->sessionById.end()){
        return ZSESSIONEXPIRED;
    }
    if(it->second.isClosing){
        return ZSESSIONEXPIRED;
    }
    if(serverId != it->second.owner){
        return ZSESSIONMOVED;
    }
    return ZOK;
}

ZOO_ERRORS setOwner(int64_t sessionId,sessionTracker * st,int64_t serverId){
    map<int64_t ,Session>::iterator it;
    it = st->sessionById.find(sessionId);
    if(it == st->sessionById.end()){
        return ZSESSIONEXPIRED;
    }
    it->second.owner = serverId;
    return ZOK;
}