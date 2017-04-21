# Coordinator
A new consensus system on RDMA

## Build
* use CLion or Idea with project name as "TkDatabase"
* use cmake

## Structure
```
- Coordinator
    - c // implementation of zookeeper's client
    - consensus // implementation of epaxos proto
       - include
           - debug.h // macros for debugging
           - tk_config.h // configurations for replica
           - tk_consensus.h // constant values 
           - tk_elog.h // command & instance strucure
           - tk_log.h // in_memory log manupilation
           - tk_message.h // messages of epaxos proto
           - tk_server.h // replica
       - src
    - coor_log // log module
       - include
           - tk_txn.h // apis for logging
       - src
    - execution // execution loop of epaxos
       - include
           - exec.h // interface for exection
       - src
    - include
       - Tkdatabase.h
       - Tkdatanode.h
    - serialization
       - include
           - protobuff.pb.h // google proto buf, don't care
           - tk_jute.h // message serializers 
       - src
    - src
    - test
    - .gitignore
    - CMakeLists.txt
    - README.md
```