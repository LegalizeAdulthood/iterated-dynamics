#!/bin/sh
#
# Build script for travis-ci.org builds to handle compiles and static
# analysis when ANALYZE=true.
#
mkdir build
cd build
if [ $ANALYZE = "true" ]; then
    if [ "$CC" = "clang" ]; then
        scan-build -h
        scan-build cmake -G "Unix Makefiles" ..
        scan-build -v -enable-checker security.insecureAPI.strcpy make
    fi
else
    cmake -G "Unix Makefiles" ..
    make
fi
