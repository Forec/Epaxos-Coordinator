#!/usr/bin/env bash
g++ -std=c++11 test_serialization.cpp ../../utils/src/communication.cpp -o test -lpthread
./test
rm test