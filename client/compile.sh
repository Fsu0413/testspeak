#!/bin/sh
${CROSS_COMPILE}g++ -c *.cpp -Iinclude --sysroot="/c/Users/Fsu04/Documents/tcpclient/toolchain/sysroot" -D__GNU_LINUX__ -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -fno-builtin-memmove --std=c++11 -fPIE -pie -Os
${CROSS_COMPILE}g++ -Wl,--no-undefined --sysroot="/c/Users/Fsu04/Documents/tcpclient/toolchain/sysroot" *.o -o test1 -fPIE -pie -Os
#${CROSS_COMPILE}ar rsv libjsoncpp.a *.o
