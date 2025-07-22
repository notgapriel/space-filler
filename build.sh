#!/bin/bash 
# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0") && \
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT") && \

cd "$SCRIPTPATH" && \
# create directory if does not already exist
mkdir -p build && \
chmod a+rwx build && \
cmake -B build -S . > build/cmake.log && \
chmod a+rwx build/Makefile && \
cd build && \
make -j $(nproc 2>/dev/null || sysctl -n hw.logicalcpu) > make.log && \
chmod a+rwx out
