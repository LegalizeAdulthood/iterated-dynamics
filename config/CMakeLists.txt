# SPDX-License-Identifier: GPL-3.0-only
#
include(CheckSymbolExists)
include(TestBigEndian)

test_big_endian(ID_BIG_ENDIAN)
check_symbol_exists(strncasecmp "strings.h" ID_HAVE_STRNCASECMP)
configure_file(port_config.h.in include/port_config.h)

add_library(config INTERFACE)
target_sources(config INTERFACE
    port_config.h.in
    include/id_io.h
    include/port.h
    include/string_case_compare.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/port_config.h"
)
target_include_directories(config INTERFACE
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")
