cmake_minimum_required(VERSION 3.6)
project(client)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        ../../database/include/Tkdatabase.h
        ../../database/src/Tkdatabase.cpp
        ../../database/include/Tkdatanode.h
        ../../database/src/Tkdatanode.cpp
        ../../consensus/include/tk_consensus.h
        ../../consensus/include/tk_config.h
        ../../consensus/include/tk_log.h
        ../../consensus/include/tk_elog.h
        ../../consensus/include/tk_command.h
        ../../consensus/include/tk_message.h
        ../../consensus/include/replica.h
        ../../consensus/src/replica.cpp
        ../../execution/include/exec.h
        ../../execution/src/exec.cpp
        ../../config/config.h
        ../../utils/include/utils.h
        ../../utils/src/utils.cpp
        ../../utils/include/msg_queue.h
        ../../utils/src/msg_queue.cpp
        ../../utils/include/communication.h
        ../../utils/src/communication.cpp
        ../include/client.h
        ../src/client.cpp)

add_executable(client ${SOURCE_FILES})
target_link_libraries(client gflags pthread)