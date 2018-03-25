cmake_minimum_required(VERSION 2.8)
project(TkDatabase)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lprotobuf -std=c++11")
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif()

set(SOURCE_FILES
        ../consensus/include/tk_consensus.h
        ../consensus/include/tk_config.h
        ../consensus/include/tk_log.h
        ../utils/include/debug.h
        ../consensus/include/tk_elog.h
        ../consensus/include/tk_command.h
        ../consensus/include/tk_message.h
        ../config/config.h
        ../utils/include/utils.h
        ../utils/src/utils.cpp
        ../utils/include/communication.h
        ../utils/src/communication.cpp
        include/protobuff.pb.h
        include/tk_jute.h
        src/protobuff.pb.cc
        src/tk_jute.cpp
        src/test.cpp)

add_executable(serializeTest ${SOURCE_FILES})

target_link_libraries(serializeTest ${PROTOBUF_LIBRARIES})
target_link_libraries(serializeTest gtest)