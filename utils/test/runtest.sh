#!/usr/bin/env bash
g++ utest.cpp ../src/msg_queue.cpp -o utest -lpthread -lev
./utest
rm utest