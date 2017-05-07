//
// Created by jingle on 17-1-3.
// Refactored by forec on 17-5-7.
//

#include "../include/Tkdatabase.h"

using namespace std;

Tkdatabase::Tkdatabase() {
    string root_path="/";
    Datanode* root_node;
    root_node = new Datanode();
    root_node->parent = NULL;
    root_node->data = new char[5];
    root_node->dataSize = 5;
    strcpy(root_node->data, "root");

    Tkdb.insert(pair<string, Datanode*>("", root_node));
    Tkdb.insert(pair<string, Datanode*>(root_path, root_node));

    string procTk_path = "/Tookeeper";
    string procChildTk = procTk_path.substr(1);
    Datanode * procTk_node = new Datanode();
    procTk_node->parent = root_node;
    procTk_node->data = new char[10];
    procTk_node->dataSize = 10;
    strcpy(procTk_node->data, "Tookeeper");

    root_node->addChild(procChildTk);
    Tkdb.insert(pair<string, Datanode*>(procTk_path, procTk_node));
}

Tkdatabase::~Tkdatabase() {
    Tkdb.clear();
}


bool Tkdatabase::insert(Datanode * node, const string & path) {
    string parent = parse_path_parent(path);
    string self = parse_path_child(path);
    auto it = Tkdb.find(parent);
    if(Tkdb.end() == it) {
        return false;
    } else {
        it->second->addChild(self);
        node->parent = it->second;
        Tkdb.insert(pair<string,Datanode *>(path, node));
        return true;
    }
}

char * Tkdatabase::fetch(const string &path, int32_t &size) {
    auto it = Tkdb.find(path);
    if(Tkdb.end() == it)
        return nullptr;
    else {
        size = it->second->dataSize;
        return it->second->data;
    }
}

int Tkdatabase::remove(const string &path) {
    string self = parse_path_child(path);
    auto it = Tkdb.find(path);
    if(Tkdb.end() == it) {
        return -2; // can not find this node.
    } else {
        if ( !(it->second->children.empty()) ) {
            return -1;  // this node has children, can not delete directly;
        } else {
            it->second->parent->children.erase(self);
            delete(it->second);
            Tkdb.erase(path);
            return 1;
        }
    }
}

bool Tkdatabase::list(const string & path, unordered_set<string> & res) {
    auto it = Tkdb.find(path);
    if( Tkdb.end() == it ) {
        return false;
    } else {
        res = it->second->getChildren();
        return true;
    }
}

bool Tkdatabase::set(const string & path, char * data, int32_t dataSize) {
    auto it = Tkdb.find(path);
    if(Tkdb.end() == it){
        return false;
    } else {
        it->second->data = data;
        it->second->dataSize = dataSize;
        return true;
    }
}

bool Tkdatabase::put(const std::string & path, char * data, int32_t dataSize) {
    if (!set(path, data, dataSize)) {
        Datanode * new_node = new Datanode();
        new_node->data = data;
        new_node->dataSize = dataSize;
        return insert(new_node, path);
    }
    return true;
}

/*******************************************************************************
 *                         Non-class Helper Functions                          *
 ******************************************************************************/

string parse_path_child(const string & node_path) {
    auto pos = node_path.find_last_of('/');
    return node_path.substr(pos + 1);
}

string parse_path_parent(const string & node_path) {
    auto pos = node_path.find_last_of('/');
    return node_path.substr(0, pos);
}
