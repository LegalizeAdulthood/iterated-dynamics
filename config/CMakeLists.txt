# SPDX-License-Identifier: GPL-3.0-only
#
include(CheckSymbolExists)
include(TestBigEndian)

test_big_endian(ID_BIG_ENDIAN)
check_symbol_exists(strncasecmp "strings.h" ID_HAVE_STRNCASECMP)
configure_file(port_config.h.in include/config/port_config.h)

check_symbol_exists(filelength "io.h" ID_HAVE_FILELENGTH_IO_H)
check_symbol_exists(filelength "unistd.h" ID_HAVE_FILELENGTH_UNISTD_H)
if(ID_HAVE_FILELENGTH_IO_H)
    configure_file(filelength.io.h.in include/config/filelength.h @ONLY)
elseif(ID_HAVE_FILELENGTH_UNISTD_H)
    configure_file(filelength.unistd.h.in include/config/filelength.h @ONLY)
else()
    message(FATAL_ERROR "Couldn't find suitable filelength function")
endif()

add_library(config INTERFACE)
target_sources(config INTERFACE
    filelength.io.h.in
    filelength.unistd.h.in
    port_config.h.in
    include/config/port.h
    include/config/string_case_compare.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/port_config.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/filelength.h"
)
target_include_directories(config INTERFACE
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")
