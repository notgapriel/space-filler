#!/bin/bash 
./build.sh && \
./build/out ${@:2,1} # all arguments for this script passed to executable
