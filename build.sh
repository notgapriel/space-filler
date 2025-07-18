#!/bin/bash 
# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0") && \
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT") && \

cd "$SCRIPTPATH" && \
# create directory if does not already exist
mkdir -p build && \
chmod a+rwx build && \
g++ -o build/out src/main.cpp -std=gnu++26 `GraphicsMagick++-config --cppflags --cxxflags --ldflags --libs`
chmod a+rwx build/out
