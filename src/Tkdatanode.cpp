//
// Created by jingle on 17-1-4.
//



#include "../include/Tkdatanode.h"


void addchild(datanode_t *root,  const string &child)
{
    cout << "add a child" << endl;
    root->children.insert(child);
}

void removechild(datanode_t *root,  const string &child)
{
    root->children.erase(child);
}



void setChildren(datanode_t *root, const unordered_set<string>& children)
{
    root->children = children;
}



unordered_set<string> getChildren(datanode_t *root)
{
    return root->children;
}


