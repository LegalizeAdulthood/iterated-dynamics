# SPDX-License-Identifier: GPL-3.0-only

cmake_minimum_required(VERSION 3.23)

# Find ZeroMQ package
find_package(ZeroMQ REQUIRED)

# Master node executable
add_executable(zmq_master
    master.cpp
)

target_link_libraries(zmq_master
    PRIVATE
        libzmq
)

set_target_properties(zmq_master PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Worker node executable
add_executable(zmq_worker
    worker.cpp
)

target_link_libraries(zmq_worker
    PRIVATE
        libzmq
)

set_target_properties(zmq_worker PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Optional: Install examples
install(TARGETS zmq_master zmq_worker
    RUNTIME DESTINATION examples/zeromq
)

install(FILES README.md
    DESTINATION examples/zeromq
)
