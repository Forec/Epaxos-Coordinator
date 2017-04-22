#!/usr/bin/env bash
g++ -std=c++11 src/*
g++ -std=c++11 consensus/src/replica.cpp
# g++ -std=c++11 coor_log/src/tk_txn.cpp
# g++ -std=c++11 serialization/src/tk_jute.cpp
g++ -std=c++11 execution/src/exec.cpp