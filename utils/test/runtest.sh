#!/usr/bin/env bash
g++ -std=c++11 utest.cpp ../src/msg_queue.cpp -o utest -lpthread -lev
./utest
rm utest
