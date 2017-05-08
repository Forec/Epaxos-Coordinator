//
// Created by jingle on 17-1-5.
// Refactored by forec on 17-5-7.
//

#include "../include/Tkdatabase.h"
#include <iostream>
#include <stdio.h>
#include <stdint.h>

int passed = 0, failed = 0;

void alert(bool conf) {
    if (conf)
        passed++;
    else
        failed++;
}

int main() {
    const char TESTDATA[9] = "DATABASE";
    std::string test_node_path = "/TEST";

    Tkdatabase *db = new Tkdatabase();

    Datanode *test_node = new Datanode();
    test_node->data = new char[9];
    test_node->dataSize = 9;
    strcpy(test_node->data, TESTDATA);

    alert( db->insert(test_node, test_node_path) );

    int resSize = 0;
    char * res = db->fetch(test_node_path, resSize);
    alert( res != nullptr );
    alert( resSize == 9 );
    alert( !strcmp(res, TESTDATA) );

    std::unordered_set<std::string> childList;
    alert( db->list("/", childList) );
    alert( childList.size() == 2 );
    alert( childList.find("TEST") != childList.end() );

    alert( 1 == db->remove(test_node_path) );

    alert( db->list("/", childList) );
    alert( childList.size() == 1 );
    alert( childList.find("TEST") == childList.end() );

    delete db;
    fprintf(stdout, "====================== TEST DATABASE ========================\n");
    fprintf(stdout, "                passed: %d, failed: %d\n", passed, failed);
    return 0;
}