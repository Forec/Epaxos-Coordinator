cmake_minimum_required(VERSION 2.8)
project(zookeeperServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")

set(SOURCE_FILES
        ../include/zk_adaptor.h
        ../src/zk_adaptor.cpp
        ../include/watch_manager.h
        ../src/watch_manager.cpp
        ../include/serverCnxn.h
        ../src/serverCnxn.cpp
        ../include/dataTree.h
        ../src/dataTree.cpp
        ../include/zookeeperServer.recordio.h
        ../src/zookeeperServer.recordio.c
        ../include/zookeeperServer.jute.h
        ../src/zookeeperServer.jute.c
        ../include/zookeeperServer.h
        ../src/zookeeperServer.cpp)

add_executable(zookeeperServer ${SOURCE_FILES})
target_link_libraries(zookeeperServer gflags pthread)
