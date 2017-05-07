//
// Created by jingle on 17-1-4.
// Refactored by forec on 17-5-7.
//

#include "../include/Tkdatanode.h"

void Datanode::addChild(const std::string & child) {
    children.insert(child);
}

void Datanode::removeChild(const std::string & child) {
    children.erase(child);
}

void Datanode::setChildren(const std::unordered_set<std::string>& children) {
    this->children = children;
}

std::unordered_set<std::string> Datanode::getChildren(){
    return children;
}


