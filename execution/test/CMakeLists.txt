cmake_minimum_required (VERSION 3.6)
project (etest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        ../../include/Tkdatabase.h
        ../../src/Tkdatabase.cpp
        ../../include/Tkdatanode.h
        ../../src/Tkdatanode.cpp
        ../../consensus/include/tk_consensus.h
        ../../consensus/include/tk_config.h
        ../../consensus/include/tk_log.h
        ../../consensus/include/debug.h
        ../../consensus/include/tk_elog.h
        ../../consensus/include/tk_message.h
        ../../consensus/include/replica.h
        ../../consensus/src/replica.cpp
        ../../execution/include/exec.h
        ../../execution/src/exec.cpp
        etest.h
        etest.cpp
  )

add_executable(etest ${SOURCE_FILES})
target_link_libraries(etest ev pthread)