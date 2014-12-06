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
          --status-bugs -v make
    else
        cd ..
        cppcheck --help
        cppcheck --template "{file}({line}): {severity} ({id}): {message}" \
            --error-exitcode=1 --enable=all --force --std=c++11 -j 4 \
            --suppress=incorrectStringBooleanError \
            --suppress=invalidscanf --inline-suppr \
            -I headers hc common headers unix win32
    fi
else
    cmake -G "Unix Makefiles" ..
    make
fi
