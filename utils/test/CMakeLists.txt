cmake_minimum_required (VERSION 3.6)
project (utest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        ../include/msg_queue.h
        ../src/msg_queue.cpp
        utest.h
        utest.cpp
  )

add_executable(utest ${SOURCE_FILES})
target_link_libraries(utest ev pthread)