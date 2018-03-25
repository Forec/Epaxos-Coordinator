cmake_minimum_required(VERSION 2.8)
project(zookeeperServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")

set(SOURCE_FILES
        ../consensus/include/tk_consensus.h
        ../consensus/include/tk_config.h
        ../consensus/include/tk_log.h
        ../consensus/include/tk_elog.h
        ../consensus/include/tk_command.h
        ../consensus/include/tk_message.h
        ../zookeeperServer/include/zk_adaptor.h
        ../zookeeperServer/src/zk_adaptor.cpp
        ../zookeeperServer/include/watch_manager.h
        ../zookeeperServer/src/watch_manager.cpp
        ../zookeeperServer/include/serverCnxn.h
        ../zookeeperServer/src/serverCnxn.cpp
        ../zookeeperServer/include/dataTree.h
        ../zookeeperServer/src/dataTree.cpp
        ../zookeeperServer/include/zookeeperServer.recordio.h
        ../zookeeperServer/src/zookeeperServer.recordio.c
        ../zookeeperServer/include/zookeeperServer.jute.h
        ../zookeeperServer/src/zookeeperServer.jute.c
        ../zookeeperServer/include/zookeeperServer.h
        ../zookeeperServer/src/zookeeperServer.cpp
        ../consensus/include/replica.h
        ../consensus/src/replica.cpp
        ../execution/include/exec.h
        ../execution/src/exec.cpp
        ../config/config.h
        ../utils/include/utils.h
        ../utils/src/utils.cpp
        ../utils/include/msg_queue.h
        ../utils/src/msg_queue.cpp
        ../utils/include/communication.h
        ../utils/src/communication.cpp
        include/tk_txn.h
        src/tk_txn.cpp
        test/test.cpp
        )

add_executable(logtest ${SOURCE_FILES})
