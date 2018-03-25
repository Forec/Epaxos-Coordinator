//
// Created by haoba on 17-8-16.
//

#include "../include/dataTree.h"
#include <string.h>
#include <iostream>
#include <vector>
using namespace std;

const int ZOO_PERM_READ = 1 << 0;
const int ZOO_PERM_WRITE = 1 << 1;
const int ZOO_PERM_CREATE = 1 << 2;
const int ZOO_PERM_DELETE = 1 << 3;
const int ZOO_PERM_ADMIN = 1 << 4;
const int ZOO_PERM_ALL = 0x1f;
struct Id ZOO_ANYONE_ID_UNSAFE = {"world", "anyone"};
struct Id ZOO_AUTH_IDS = {"auth", ""};
static struct ACL _OPEN_ACL_UNSAFE_ACL[] = {{0x1f, {"world", "anyone"}}};
static struct ACL _READ_ACL_UNSAFE_ACL[] = {{0x01, {"world", "anyone"}}};
static struct ACL _CREATOR_ALL_ACL_ACL[] = {{0x1f, {"auth", ""}}};
struct ACL_vector ZOO_OPEN_ACL_UNSAFE = { 1, _OPEN_ACL_UNSAFE_ACL};
struct ACL_vector ZOO_READ_ACL_UNSAFE = { 1, _READ_ACL_UNSAFE_ACL};
struct ACL_vector ZOO_CREATOR_ALL_ACL = { 1, _CREATOR_ALL_ACL_ACL};


bool checkACL(struct ACL_vector acls,int32_t perms,vector<struct Id> auth){
    if(acls.count == 0){
        return true;
    }
    char superScheme[] = "super";
    for(int32_t i = 0;i < auth.size();i++){
        if(strcmp(superScheme,auth.at(i).scheme) == 0)
            return true;
    }
    char world[] = "world";
    char anyone[] = "anyone";
    //cout<<"acl count"<<acls.count<<endl;
    //cout<<"auth count"<<auth.size()<<endl;
    for(int32_t i = 0;i < acls.count;i++){
        struct Id id = acls.data[i].id;
        if((acls.data[i].perms & perms) != 0){
            if((strcmp(world,id.scheme) == 0) && (strcmp(anyone,id.id) == 0)){
                //cout<<"world anyone"<<endl;
                return true;
            }
            for(int32_t j = 0;j < auth.size();j++){
                //cout<<"scheme"<<auth.at(j).scheme<<" "<<id.scheme<<endl;
                //cout<<"id"<<auth.at(j).id<<" "<<id.id<<endl;
                //cout<<strcmp(auth.at(j).scheme,id.scheme)<<endl;
                //cout<<strcmp(auth.at(j).id,id.id)<<endl;
                if((strcmp(auth.at(j).scheme,id.scheme) == 0 ) && (strcmp(auth.at(j).id,id.id) == 0)){
                    return true;
                }
            }
        }
    }

    return false;
}

void copyStat(struct Stat * stat,dataNode * node){
    stat->ctime = node->stat.ctime;
    stat->mtime = node->stat.mtime;
    stat->aversion = node->stat.aversion;
    stat->czxid = node->stat.czxid;
    stat->mtime = node->stat.mtime;
    stat->mzxid = node->stat.mzxid;
    stat->pzxid = node->stat.pzxid;
    stat->ephemeralOwner = node->stat.ephemeralOwner;
    stat->dataLength = node->data.len;
    int numChildren = node->son.size();
    stat->numChildren = numChildren;
    stat->cversion = node->stat.cversion*2 - numChildren;
}

vector<string> getPathVector(string path){
    vector<string> temp;
    string nextPath = path.substr(1,path.length());
    int32_t locate = nextPath.find('/');
    while(locate > 0){
        string name = nextPath.substr(0,locate);
        temp.push_back(name);
        nextPath = nextPath.substr(locate+1,nextPath.length());
        locate = nextPath.find('/');
    }
    temp.push_back(nextPath);
    return temp;
}



ZOO_ERRORS zoo_addNode(dataTree * dt,char * path,struct buffer data,struct ACL_vector acls,
                 int64_t ephemeralOwner,int32_t flag, int64_t zxid, int64_t time,vector<struct Id> auth){
    string nodePath(path);
    cout<<nodePath<<endl;
    bool permited = false;
    if(nodePath == "/"){
        return ZBADARGUMENTS;
    }else{
        cout<<"starting add node"<<endl;
        vector<string> vPath = getPathVector(nodePath);
        dataNode * nextNode = &dt->root;
        //cout<<vPath.size()<<endl;
        for(int i = 0;i < vPath.size();i++){
            map<string ,dataNode *>::iterator l_it;
            l_it=nextNode->son.find(vPath.at(i));
            //cout<<vPath.at(i)<<endl;
            if(l_it==nextNode->son.end()){
                //if(i != vPath.size()-1){
                //    return ZNONODE;
                //}
                //create new node
                if(!permited){
                    if(checkACL(nextNode->acl,ZOO_PERM_CREATE,auth))
                        permited = true;
                    else{
                        cout<<"create node permission denied"<<endl;
                        return ZNOAUTH;
                    }
                }
                nextNode->stat.cversion++;
                dataNode * newNode = new dataNode;
                newNode->parentNode = nextNode;
                newNode->stat.ctime = time;
                newNode->stat.mtime = time;
                newNode->stat.czxid = zxid;
                newNode->stat.mzxid = zxid;
                newNode->stat.pzxid = zxid;
                newNode->stat.aversion = 0;
                newNode->stat.cversion = 0;
                newNode->stat.ephemeralOwner = 0;
                newNode->acl = acls;
                if(i == vPath.size()-1){
                    newNode->data = data;
                }
                newNode->path = (char *)malloc(nodePath.size());
                memcpy(newNode->path,path,nodePath.size());
                if(flag == EPHEMERAL || flag == EPHEMERAL_SEQUENTIAL){
                    newNode->stat.ephemeralOwner = ephemeralOwner;
                    if(dt->ephemerals.find(ephemeralOwner) != dt->ephemerals.end()){
                        dt->ephemerals.at(ephemeralOwner).insert(nodePath);
                    }else{
                        set<string> eph;
                        eph.insert(nodePath);
                        dt->ephemerals.insert(make_pair(ephemeralOwner,eph));
                    }
                }
                nextNode->son.insert(pair<string,dataNode*>(vPath.at(i),newNode));
                cout<<"add new node "<<vPath.at(i)<<endl;
                nextNode = nextNode->son.at(vPath.at(i));
            }else{
                if(i == vPath.size()){
                    return ZNODEEXISTS;
                }
                nextNode = l_it->second;
            }
        }
    }
    cout<<"add node complete"<<endl;
    return ZOK;
}



ZOO_ERRORS zoo_isExist(dataTree * dt,char * path,struct ExistsResponse * er){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
            copyStat(&er->stat,nextNode);
            cout<<"is exist"<<nextNode->data.buff<<endl;
        }
    }
    return ZOK;
}

ZOO_ERRORS zoo_getChildren(dataTree * dt,char * path,struct String_vector * children,vector<struct Id> auth){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    if(checkACL(nextNode->acl,ZOO_PERM_READ,auth)){
        int32_t count = nextNode->son.size();
        children->count = count;
        int32_t i = 0;
        map<string,dataNode*>::iterator it;
        for(it = nextNode->son.begin();it != nextNode->son.end();it++){
            string child = it->first;
            children->data[i] = (char *)malloc(child.length()+1);
            memcpy(children->data[i],child.c_str(),child.length());
            children->data[i][child.length()] = '\0';
            i++;
        }
        return ZOK;
    }else{
        cout<<"getchildren perm denied"<<endl;
        return ZNOAUTH;
    }
}

ZOO_ERRORS zoo_getData(dataTree * dt,char * path,struct GetDataResponse * gdr,vector<struct Id> auth){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        cout<<"get data path "<<vPath.at(i)<<endl;
        cout<<"path son size="<<nextNode->son.size()<<endl;
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            cout<<"find path "<<vPath.at(i)<<endl;
            nextNode = l_it->second;
        }
    }
    if(!checkACL(nextNode->acl,ZOO_PERM_READ,auth)){
        cout<<"get data perms denied"<<endl;
        return ZNOAUTH;
    }
    gdr->data = nextNode->data;
    copyStat(&gdr->stat,nextNode);
    return ZOK;
}


ZOO_ERRORS zoo_setData(dataTree * dt,char * path,struct buffer data, int32_t version, int64_t zxid, int64_t time,struct Stat * stat,vector<struct Id> auth){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    if(checkACL(nextNode->acl,ZOO_PERM_WRITE,auth)){
        if(version != -1 && version != nextNode->stat.version){
            return ZBADVERSION;
        }
        nextNode->data = data;
        nextNode->stat.mzxid = zxid;
        nextNode->stat.mtime = time;
        nextNode->stat.version++;
        copyStat(stat,nextNode);
        return ZOK;
    }
    cout<<"set data permission denied"<<endl;
    return ZNOAUTH;
}

ZOO_ERRORS zoo_deleteNode(dataTree * dt, char * path,int64_t zxid,vector<struct Id> auth){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);

    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
            if(i == vPath.size()-1){
                if(nextNode->son.size() != 0){
                    cout<<"delete node denied : has children"<<endl;
                    return ZNOTEMPTY;
                }
                if(checkACL(nextNode->parentNode->acl,ZOO_PERM_DELETE,auth)){
                    nextNode->parentNode->stat.pzxid = zxid;
                    int64_t ephemeral = nextNode->stat.ephemeralOwner;
                    if(ephemeral != 0){
                        dt->ephemerals.at(ephemeral).erase(nodePath);
                    }
                    nextNode->parentNode->son.erase(vPath.at(vPath.size()-1));
                    delete nextNode;
                    cout<<"delete node complete"<<endl;
                    return ZOK;
                }else {
                    cout<<"delete node perm denied"<<endl;
                    return ZNOAUTH;
                }
            }
        }
    }
    return ZOK;
}

ZOO_ERRORS zoo_setACL(dataTree * dt,char * path,struct ACL_vector acls,int32_t version,vector<struct Id> auth,struct Stat * stat){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    if(checkACL(nextNode->acl,ZOO_PERM_ADMIN,auth)){
        if(version != -1 && version != nextNode->stat.aversion){
            return ZBADVERSION;
        }
        nextNode->acl = acls;
        nextNode->stat.aversion++;
        copyStat(stat,nextNode);
        return ZOK;
    }else{
        cout<<"set acl perm denied"<<endl;
        return ZNOAUTH;
    }

}

ZOO_ERRORS zoo_getACL(dataTree * dt,char * path,struct GetACLResponse * gar){
    string nodePath(path);
    vector<string> vPath = getPathVector(nodePath);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    gar->acl = nextNode->acl;
    copyStat(&gar->stat,nextNode);
    return ZOK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ZOO_ERRORS zoo_getData(dataTree * dt,string path,struct GetDataResponse * gdr,vector<struct Id> auth){
    vector<string> vPath = getPathVector(path);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    if(!checkACL(nextNode->acl,ZOO_PERM_READ,auth)){
        cout<<"get data perms denied"<<endl;
        return ZNOAUTH;
    }
    gdr->data = nextNode->data;
    copyStat(&gdr->stat,nextNode);
    return ZOK;
}

ZOO_ERRORS zoo_addNode(dataTree * dt,string path,struct buffer data,struct ACL_vector acls,
                       int64_t ephemeralOwner,int32_t flag, int64_t zxid, int64_t time,vector<struct Id> auth){
    cout<<"add node :path="<<path<<endl;
    bool permited = false;
    if(path == "/"){
        return ZBADARGUMENTS;
    }else{
        vector<string> vPath = getPathVector(path);
        dataNode * nextNode = &dt->root;
        //cout<<vPath.size()<<endl;
        for(int i = 0;i < vPath.size();i++){
            map<string ,dataNode *>::iterator l_it;
            l_it=nextNode->son.find(vPath.at(i));
            cout<<vPath.at(i)<<endl;
            if(l_it==nextNode->son.end()){
                //if(i != vPath.size()-1){
                //    return ZNONODE;
                //}
                //create new node
                if(!permited){
                    if(checkACL(nextNode->acl,ZOO_PERM_CREATE,auth))
                        permited = true;
                    else{
                        cout<<"create node permission denied"<<endl;
                        return ZNOAUTH;
                    }
                }
                nextNode->stat.cversion++;
                dataNode * newNode = new dataNode;
                newNode->parentNode = nextNode;
                newNode->stat.ctime = time;
                newNode->stat.mtime = time;
                newNode->stat.czxid = zxid;
                newNode->stat.mzxid = zxid;
                newNode->stat.pzxid = zxid;
                newNode->stat.aversion = 0;
                newNode->stat.cversion = 0;
                newNode->stat.ephemeralOwner = 0;
                newNode->acl = acls;
                newNode->data = data;
                newNode->path = (char*)malloc(path.size());
                memcpy(newNode->path,path.c_str(),path.size());
                if(flag == EPHEMERAL || flag == EPHEMERAL_SEQUENTIAL){
                    newNode->stat.ephemeralOwner = ephemeralOwner;
                    if(dt->ephemerals.find(ephemeralOwner) != dt->ephemerals.end()){
                        dt->ephemerals.at(ephemeralOwner).insert(path);
                    }else{
                        set<string> eph;
                        eph.insert(path);
                        dt->ephemerals.insert(make_pair(ephemeralOwner,eph));
                    }
                }
                nextNode->son.insert(pair<string,dataNode*>(vPath.at(i),newNode));
                nextNode = nextNode->son.at(vPath.at(i));
            }else{
                if(i == vPath.size()){
                    return ZNODEEXISTS;
                }
                nextNode = l_it->second;
            }
        }
    }
    cout<<"add node complete"<<endl;
    return ZOK;
}

ZOO_ERRORS zoo_setData(dataTree * dt,string path,struct buffer data, int32_t version, int64_t zxid, int64_t time,struct Stat * stat,vector<struct Id> auth){
    vector<string> vPath = getPathVector(path);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
        }
    }
    if(checkACL(nextNode->acl,ZOO_PERM_WRITE,auth)){
        if(version != -1 && version != nextNode->stat.version)
            return ZBADVERSION;
        nextNode->data = data;
        nextNode->stat.mzxid = zxid;
        nextNode->stat.mtime = time;
        nextNode->stat.version++;
        copyStat(stat,nextNode);
        return ZOK;
    }
    cout<<"set data permission denied"<<endl;
    return ZNOAUTH;
}

ZOO_ERRORS zoo_isExist(dataTree * dt,string path,struct ExistsResponse * er){
    vector<string> vPath = getPathVector(path);
    dataNode * nextNode = &dt->root;
    for(int i = 0;i < vPath.size();i++){
        map<string ,dataNode *>::iterator l_it;
        l_it=nextNode->son.find(vPath.at(i));
        if(l_it==nextNode->son.end()){
            return ZNONODE;
        }else{
            nextNode = l_it->second;
            copyStat(&er->stat,nextNode);
            cout<<"is exist"<<nextNode->data.buff<<endl;
        }
    }
    return ZOK;
}