#!/bin/sh
g++ -c *.cpp -Iinclude -g -static -static-libgcc -static-libstdc++ --std=c++11 -O2
g++ -Wl,--no-undefined *.o -o test1 -O2 -g --static -static-libgcc -static-libstdc++ -lwsock32 -lws2_32
