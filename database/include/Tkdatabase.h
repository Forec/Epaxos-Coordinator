//
// Created by jingle on 17-1-3.
// Refactored by forec on 17-5-7.
//

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Tkdatanode.h"

struct Tkdatabase {
    std::unordered_map <std::string, Datanode*> Tkdb;
    Tkdatabase();
    ~Tkdatabase();
    char * fetch(const std::string & path, int32_t & size);
    bool insert(Datanode * node, const std::string & path);
    bool list(const std::string & path, std::unordered_set<std::string> & res);
    bool set(const std::string & path, char * data, int32_t dataSize);
    bool put(const std::string & path, char * data, int32_t dataSize);
    int remove(const std::string &path);
};

// non-class helper functions
std::string parse_path_child(const std::string & node_path);
std::string parse_path_parent(const std::string & node_path);