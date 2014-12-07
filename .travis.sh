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
        scan-build -enable-checker security.FloatLoopCounter \
          -enable-checker security.insecureAPI.UncheckedReturn \
          --status-bugs -v \
          make -j 8
    else
        cd ..
        cppcheck --help
        cppcheck --template "{file}({line}): {severity} ({id}): {message}" \
            --enable=style --force --std=c++11 -j 8 \
            --suppress=incorrectStringBooleanError \
            --suppress=invalidscanf --inline-suppr \
            -I headers hc common headers unix win32 2> cppcheck.txt
        if [ -s cppcheck.txt ]; then
            cat cppcheck.txt
            exit 1
        fi
    fi
else
    cmake -G "Unix Makefiles" ..
    make
fi
