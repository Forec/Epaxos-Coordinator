#!/usr/bin/env bash
g++ -std=c++11 utest.cpp ../src/msg_queue.cpp ../src/utils.cpp -o utest -lpthread -lev
./utest
rm utest
