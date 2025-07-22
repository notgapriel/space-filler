#!/bin/bash 
./build.sh && \
./build/out/space-filler ${@:2,1} # all arguments for this script passed to executable
