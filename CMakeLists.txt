cmake_minimum_required(VERSION 3.6)
project(TkDatabase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        include/Tkdatabase.h
        include/Tkdatanode.h
        consensus/include/tk_consensus.h
        consensus/include/tk_config.h
        consensus/include/tk_log.h
        consensus/include/debug.h
        consensus/include/tk_elog.h
        consensus/include/tk_server.h
        consensus/include/tk_message.h
        execution/include/exec.h
        src/Tkdatabase.cpp
        src/Tkdatanode.cpp
        test/add_node_test.cpp
        consensus/src/replica.cpp
        execution/src/exec.c)
add_executable(TkDatabase ${SOURCE_FILES})