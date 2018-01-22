#!/bin/sh
g++ -c *.cpp -Iinclude -D__GNU_LINUX__ -g --std=c++11 -O2
g++ *.o -o test1 -O2 -g 
