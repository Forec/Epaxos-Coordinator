cmake_minimum_required(VERSION 3.6)
project(TkDatabase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        include src
        consensus/include consensus/src
        c/generated
        execution/include execution/src
        serialization/include serialization/src
        coor_log/include coor_log/src)

set(SOURCE_FILES
        include/Tkdatabase.h
        src/Tkdatabase.cpp
        include/Tkdatanode.h
        src/Tkdatanode.cpp
        consensus/include/tk_consensus.h
        consensus/include/tk_config.h
        consensus/include/tk_log.h
        consensus/include/debug.h
        consensus/include/tk_elog.h
        consensus/include/tk_server.h
        consensus/include/tk_message.h
        consensus/src/replica.cpp
        execution/include/exec.h
        execution/src/exec.c)
add_executable(TkDatabase ${SOURCE_FILES})