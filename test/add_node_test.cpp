//
// Created by jingle on 17-1-5.
//

#include "../include/Tkdatanode.h"
#include "../include/Tkdatabase.h"

#include <boost/serialization/unordered_map.hpp>


char *getRandomString(int len, unsigned seed)
{
    int flag, i;

    srand((unsigned) time(NULL) + seed);

    //printf("%llu\n",(unsigned long long) getTime());

    char *string;
    if ((string = (char *) malloc(sizeof(char) * len)) == NULL){

        printf("Malloc Failed \n");
        return NULL;
    }
    for (i = 0; i < len - 1; i++){
        flag = rand() % 3;

        switch (flag)
        {
            case 0:
                string[i] = 'A' + rand() % 26;
                break;
            case 1:
                string[i] = 'a' + rand() % 26;
                break;
            case 2:
                string[i] = '0' + rand() % 10;
                break;
            default:
                string[i] = 'x';
                break;

        }
    }

    string [len - 1] = '\0';
    return string;

}

char **getNodeContent(int nodesnum, int content_len){

    int i;
    char **content;

    content = (char **)malloc(sizeof(char*) * nodesnum);


    for(i = 0; i < nodesnum; i++){

        *(content + i) = getRandomString(content_len, (unsigned) i);

    }

    return content;

}

char **getNodesPath(int filesnum, const char *base_dir){

    int i;
    char **filepath;

    filepath = (char **)malloc(sizeof(char *) * filesnum);



    char str[25];

    for (i = 0; i < filesnum; i++){
        //char *filename = base_dir;
        char filename[100];
        *(filepath + i) = (char *)malloc(sizeof(char) * 125);

        strcpy(filename, base_dir);

        sprintf(str, "%d", i);
        strcat(filename, "/");
        strcat(filename, str);

        strcpy(*(filepath + i), filename);
//	*(filepath + i) = filename;

    }

    return filepath;


}

int main()
{
    Tkdatabase_t *db = new Tkdatabase_t();

    init_tkdatabase(db);

    datanode_t *node = new datanode_t();

    node->data = "test";

    string path = "/wang";

    add_node_to_db(db,  node, path);
    char *res;
    int ret = getdata_from_db(db, path, res);

    if(ret){

        cout << "Get data Successfully!" << endl;
        cout << "Content:" << res << endl;
    }

    int nodesnum = 100;

    char **nodecontent = getNodeContent(nodesnum, 1024 * 4);

    char **nodepath = getNodesPath(nodesnum, path.data());

    for(int i = 0; i < nodesnum; i++){

        datanode_t *test = new datanode_t();

        test->data = *(nodecontent + i);

        string test_path = string(*(nodepath + i));
        cout << test_path << endl;
        add_node_to_db(db, test, test_path);

    }

    unordered_set<string> ss;

    int re = getchildren_from_db(db, path, ss);
    cout << re << endl;
    if(re){
        cout << ss.size() << endl;
//        for(auto it = ss.cbegin(); it != ss.cend(); ++it){
//            cout << it->data() << endl;
//        }
    }
    setData_to_datanode(db, "/wang/89", "wangjiang");


    for(int i = 0; i < 90; i++){
        char *sss;
        int va = getdata_from_db(db, *(nodepath + i), sss);
        if (va){
            cout << "path:"<<*(nodepath + i) <<";content:" << sss << endl;
        }

    }

    for(int i = 0; i < nodesnum; i++){

        del_node_in_db(db, *(nodepath + i));
    }

    re = getchildren_from_db(db, path, ss);
    if(re){
        cout << ss.size() << endl;
//        for(auto it = ss.cbegin(); it != ss.cend(); ++it){
//            cout << it->data() << endl;
//        }
    }



    destroy_tkdatabase(db);


}
