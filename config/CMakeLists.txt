# SPDX-License-Identifier: GPL-3.0-only
#
include(CheckSymbolExists)
include(TestBigEndian)

test_big_endian(ID_BIG_ENDIAN)
check_symbol_exists(strncasecmp "strings.h" ID_HAVE_STRNCASECMP)
configure_file(port_config.h.in include/config/port_config.h)

check_symbol_exists(filelength "io.h" ID_HAVE_FILELENGTH_IO_H)
if(ID_HAVE_FILELENGTH_IO_H)
    configure_file(filelength.io.h.in include/config/filelength.h @ONLY)
else()
    configure_file(filelength.stat.h.in include/config/filelength.h @ONLY)
endif()

set(ID_HAVE_CURSES_DRIVER OFF)
set(ID_HAVE_X11_DRIVER OFF)
set(ID_HAVE_GDI_DRIVER OFF)
set(ID_HAVE_WIN32_DISK_DRIVER OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_HAVE_GDI_DRIVER ON)
    set(ID_HAVE_WIN32_DISK_DRIVER ON)
    configure_file(path_limits.windows.h.in include/config/path_limits.h @ONLY)
    configure_file(fdio.windows.h.in include/config/fdio.h @ONLY)
    configure_file(getpid.windows.h.in include/config/getpid.h @ONLY)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ID_HAVE_X11_DRIVER ON)
    configure_file(path_limits.linux.h.in include/config/path_limits.h @ONLY)
    configure_file(fdio.linux.h.in include/config/fdio.h @ONLY)
    configure_file(getpid.linux.h.in include/config/getpid.h @ONLY)
else()
    message(FATAL_ERROR "Unknown sytem name '${CMAKE_SYSTEM_NAME}'; expecting 'Windows' or 'Linux'")
endif()
configure_file(driver_types.h.in include/config/driver_types.h @ONLY)

add_library(config INTERFACE)
target_sources(config INTERFACE
    driver_types.h.in
    fdio.linux.h.in
    fdio.windows.h.in
    filelength.io.h.in
    filelength.stat.h.in
    getpid.linux.h.in
    getpid.windows.h.in
    path_limits.linux.h.in
    path_limits.windows.h.in
    port_config.h.in
    include/config/port.h
    include/config/string_case_compare.h
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/driver_types.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/fdio.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/filelength.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/getpid.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/path_limits.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/port_config.h"
)
target_include_directories(config INTERFACE
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")

add_subdirectory(tests)
