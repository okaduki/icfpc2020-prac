#!/bin/sh

topdir=$(dirname $0)
cd $topdir

cd app
mkdir -p ../build

CFLAGS="-g -Wall -O0"
# g++ -std=c++14 $CFLAGS -o ../build/main main.cpp
g++ -std=c++14 $CFLAGS -o ../build/parser parser.cpp -lcurl