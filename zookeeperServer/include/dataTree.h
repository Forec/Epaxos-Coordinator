//
// Created by haoba on 17-8-16.
//

#ifndef TKDATABASE_DATATREE_H
#define TKDATABASE_DATATREE_H

#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <iostream>
#include "zookeeperServer.jute.h"
#include "zookeeperServer.recordio.h"
#include "proto.h"
#include "watch_manager.h"


/**
 * @name ACL Consts
 */
extern const int ZOO_PERM_READ;
extern const int ZOO_PERM_WRITE;
extern const int ZOO_PERM_CREATE;
extern const int ZOO_PERM_DELETE;
extern const int ZOO_PERM_ADMIN;
extern const int ZOO_PERM_ALL;

/** This Id represents anyone. */
extern struct Id ZOO_ANYONE_ID_UNSAFE;
/** This Id is only usable to set ACLs. It will get substituted with the
 * Id's the client authenticated with.
 */
extern struct Id ZOO_AUTH_IDS;

/** This is a completely open ACL*/
extern struct ACL_vector ZOO_OPEN_ACL_UNSAFE;
/** This ACL gives the world the ability to read. */
extern struct ACL_vector ZOO_READ_ACL_UNSAFE;
/** This ACL gives the creators authentication id's all permissions. */
extern struct ACL_vector ZOO_CREATOR_ALL_ACL;

struct dataNode_t{
    struct dataNode_t * parentNode;
    struct StatPersisted stat;
    struct ACL_vector acl;
    struct buffer data;
    std::map<std::string,struct dataNode_t *> son;
};

typedef dataNode_t dataNode;

struct dataTree_t{
    dataNode root;
    unordered_map<int64_t ,set<string>> ephemerals;
    watchManager dataWatch;
    watchManager childWatch;
};

typedef dataTree_t dataTree;


ZOO_ERRORS zoo_addNode(dataTree * dt,char * path,struct buffer data,struct ACL_vector acls,
                 int64_t ephemeralOwner, int32_t flag, int64_t zxid, int64_t time,std::vector<struct Id> auth);
ZOO_ERRORS zoo_isExist(dataTree * dt,char * path,struct ExistsResponse * er);
ZOO_ERRORS zoo_deleteNode(dataTree * dt, char * path,int64_t zxid,std::vector<struct Id> auth);
ZOO_ERRORS zoo_getChildren(dataTree * dt,char * path,struct String_vector * children,std::vector<struct Id> auth);
ZOO_ERRORS zoo_getACL(dataTree * dt,char * path,struct GetACLResponse * gar);
ZOO_ERRORS zoo_setACL(dataTree * dt,char * path,struct ACL_vector acls,int32_t version,std::vector<struct Id> auth,struct Stat * stat);
ZOO_ERRORS zoo_setData(dataTree * dt,char * path,struct buffer data, int32_t version, int64_t zxid, int64_t time,struct Stat * stat,std::vector<struct Id> auth);
ZOO_ERRORS zoo_getData(dataTree * dt,char * path,struct GetDataResponse * gdr,std::vector<struct Id> auth);



#endif //TKDATABASE_DATATREE_H
