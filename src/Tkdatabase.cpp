//
// Created by jingle on 17-1-3.
//



#include "../include/Tkdatanode.h"


void init_tkdatabase(Tkdatabase_t *db)

{
    cout <<"init the Tkdatabase" << endl;
    string root_path="/";

    datanode_t* root_node;

    root_node = new datanode_t();
    root_node->parent = NULL;
    root_node->data = "root";

    db->Tkdb.insert(pair<string, datanode_t*>("", root_node));

    db->Tkdb.insert(pair<string, datanode_t*>(root_path, root_node));

    string procTk_path = "/Tookeeper";

    string procChildTk = procTk_path.substr(1);

    datanode_t * procTk_node = new datanode_t();
    procTk_node->parent = root_node;

    procTk_node->data =  "Tookeeper";

    addchild(root_node, procChildTk);
    db->Tkdb.insert(pair<string, datanode_t*>(procTk_path, procTk_node));
    cout <<"init compelete!" << endl;
}

void destroy_tkdatabase(Tkdatabase_t *db)
{
    cout << "destroy the database" << endl;
//    for (auto it = db->Tkdb.cbegin(); it != db->Tkdb.cend(); ++it){
//        delete(it->second);
//    }
    db->Tkdb.clear();
    db->ephemerals.clear();
    delete(db);
}

string parse_path_child(const string & node_path)
{
    long pos = node_path.find_last_of('/');

    return node_path.substr(pos + 1);
}

string parse_path_parent(const string & node_path)
{
    long pos = node_path.find_last_of('/');

    return node_path.substr(0, pos);
}


int add_node_to_db(Tkdatabase_t* db, datanode_t *datanode, const string & node_path)
{
    string parent = parse_path_parent(node_path);
    string self = parse_path_child(node_path);
    unordered_map<string, datanode_t*>::const_iterator it = db->Tkdb.find(parent);
    if(db->Tkdb.end() == it){
        return -1;
    }else{
        addchild(it->second, self);
        datanode->parent = it->second;
        db->Tkdb.insert(pair<string,datanode_t *>(node_path, datanode));
        return 1;
    }
}


int getdata_from_db(const Tkdatabase_t *db, const char * node_path_cstr, char** getdata)
{
    string node_path(node_path_cstr);
    unordered_map<string, datanode_t*>::const_iterator it = db->Tkdb.find(node_path);
    cout << "Welcome to get data" << endl;
    if(db->Tkdb.end() == it){
        cout<<"Find failed" << endl;
        return -1;
    }else{
        *getdata = it->second->data;
        return 1;
    }

}

int del_node_in_db(Tkdatabase_t *db, const string &node_path)
{

    //string parent  = parse_path_parent(node_path);
    string self = parse_path_child(node_path);

    unordered_map<string, datanode_t*>::const_iterator it = db->Tkdb.find(node_path);

    if(it == db->Tkdb.cend()){
        return -2; //can not find this node.
    }else{
        if (!(it->second->children.empty())){

            return -1; //this node has chilren, can not delete directly;
        }else{

            it->second->parent->children.erase(self);

            //delete(it->second->parent);
            delete(it->second);
            db->Tkdb.erase(node_path);
            return 1;
        }

    }

}

int getchildren_from_db(Tkdatabase_t *db, const string & path, unordered_set<string> & res)
{
    auto it = db->Tkdb.find(path);

    if(it == db->Tkdb.cend()){
        cout <<"can not find this node" << endl;
        return -1;
    }else {

        res = getChildren(it->second);
        return 1;
    }
}


int setData_to_datanode(Tkdatabase_t *db, const string & path, char *data)
{
    auto it = db->Tkdb.find(path);

    if(it == db->Tkdb.cend()){

        cout << "Can not find this node, set operation failed" << endl;
        return -1;
    }else{
        it->second->data = data;
        return 1;
    }
}

int putData_into_db(Tkdatabase_t *db, const char *path, char *data)
{
    int res;
    string path_str(path);
    if (-1 == (res = setData_to_datanode(db, path_str, data))) {
        datanode_t * new_node = new datanode_t();
        new_node->data = data;
        return add_node_to_db(db, new_node, path_str);
    }
    return res;
}


struct test{

    unordered_map <string, char *> kk;
    unordered_set <string> ss;
};

typedef struct test test_t;


//int main()
//{
//
//    string path = "/Tookeeper";
//    string test_node = "/wangjiang";
//
//
//    //Tkdatabase_t *db = (Tkdatabase_t*)malloc(sizeof(Tkdatabase_t));
//    Tkdatabase_t *db = new Tkdatabase_t();
//    init_tkdatabase(db);
//    datanode_t *test = new datanode_t();
//
//    test->data = "waddddd";
//
//    add_node_to_db(db, test,test_node);
//
//    char * res = "";
//
//    int ret = getdata_from_db(db, test_node, res);
//
//    if(ret){
//
//        cout << "Get Data Successfully" << endl;
//        cout <<"content:" << res << endl;
//    }
//
//    ret = del_node_in_db(db, test_node);
//
//    if(ret == 1){
//
//        cout <<"Delete data Successfully" << endl;
//    }
//    else if(ret == -2){
//        cout <<"Can not find this node" << endl;
//    }
//    else if(ret == -1){
//        cout <<"This node has children can not be delete" << endl;
//    }
////      test_t *ptr = (test_t*)malloc(sizeof(test_t));
////      ptr->kk["wangjiang"] = "root";
////     // ptr->kk.insert(pair<string, char*> ("wangjiang", "root"));
////
////      cout <<ptr->kk["wangjiang"]<<endl;
////    string res = parse_path_child(path);
////    string root = parse_path_parent(path);
////    map <string, int> map1;
////
////    map1.insert(pair<string, int>("wangjiang", 1));
////
////    set <string> set1;
////
////    set1.insert("wangjiang");
//
////    cout <<*set1.find("wangjiang")<< endl;
//
////    cout << map1["wangjiang"] << endl;
//    //printf("%x", res);
////    cout << res << endl;
////    cout << root << endl;
//    destroy_tkdatabase(db);
//    return 0;
//
//}