//
// Created by jingle on 17-1-4.
//

#ifndef TKDATABASE_TKDATANODE_H
#define TKDATABASE_TKDATANODE_H


#include "Tkdatabase.h"


void addchild(datanode_t *root, const string &child);

void removechild(datanode_t *root, const string &child);



void setChildren(datanode_t *root, const unordered_set<string>& children);



unordered_set<string> getChildren(datanode_t *root);


#endif //TKDATABASE_TKDATANODE_H
