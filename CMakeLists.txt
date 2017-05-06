cmake_minimum_required(VERSION 3.6)
project(TkDatabase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        include
        consensus/include
        c/generated
        execution/include
        serialization/include
        coor_log/include)

set(SOURCE_FILES
        include/Tkdatabase.h
        src/Tkdatabase.cpp
        include/Tkdatanode.h
        src/Tkdatanode.cpp
        utils/include/msg_queue.h
        utils/src/msg_queue.cpp
        consensus/include/tk_consensus.h
        consensus/include/tk_config.h
        consensus/include/tk_log.h
        consensus/include/debug.h
        consensus/include/tk_elog.h
        consensus/include/tk_command.h
        consensus/include/tk_message.h
        consensus/include/replica.h
        consensus/src/replica.cpp
        execution/include/exec.h
        execution/src/exec.cpp
        config/config.h
        utils/include/go_thread.h
        utils/src/go_thread.cpp
        utils/include/utils.h
        utils/src/utils.cpp)

add_executable(TkDatabase ${SOURCE_FILES})