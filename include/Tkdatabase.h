//
// Created by jingle on 17-1-3.
//




#include <stdint-gcc.h>
#include <iostream>
#include <string.h>
#include <set>
#include <string>
//#include <ext/hash_map>
//#include <ext/hash_set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "Tkdatanode.h"
//#include "../c/generated/zookeeper.jute.h"

using namespace std;

//using namespace __gnu_cxx;


struct Tkdatabase {

    unordered_map <string, datanode_t*> Tkdb;

    unordered_map <uint64_t, unordered_set<string>> ephemerals;

};

typedef struct Tkdatabase Tkdatabase_t;




void init_tkdatabase(Tkdatabase_t *db);

void destroy_tkdatabase(Tkdatabase_t *db);

string parse_path_child(const string & node_path);

string parse_path_parent(const string & node_path);

int add_node_to_db(Tkdatabase_t* db, datanode_t *datanode, const string & node_path);

int getdata_from_db(const Tkdatabase_t *db, const char *node_path_cstr, char** getdata);

int del_node_in_db(Tkdatabase_t *db, const string &node_path);

int getchildren_from_db(Tkdatabase_t *db, const string & path, unordered_set<string> & res);

int setData_to_datanode(Tkdatabase_t *db, const string & path, char *data);

int putData_into_db(Tkdatabase_t *db, const char *path, char *data);