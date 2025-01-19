# SPDX-License-Identifier: GPL-3.0-only
#
include(CheckSymbolExists)
include(TestBigEndian)

test_big_endian(ID_BIG_ENDIAN)
configure_file(port_config.h.in include/config/port_config.h)

set(ID_HAVE_CURSES_DRIVER OFF)
set(ID_HAVE_X11_DRIVER OFF)
set(ID_HAVE_GDI_DRIVER OFF)
set(ID_HAVE_WIN32_DISK_DRIVER OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(ID_HAVE_GDI_DRIVER ON)
    set(ID_HAVE_WIN32_DISK_DRIVER ON)
    set(ID_STRING_LOWER_SOURCES "")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(ID_HAVE_X11_DRIVER ON)
    set(ID_STRING_LOWER_SOURCES "string_lower.cpp")
else()
    message(FATAL_ERROR "Unknown sytem name '${CMAKE_SYSTEM_NAME}'; expecting 'Windows' or 'Linux'")
endif()
configure_file(driver_types.h.in                        include/config/driver_types.h @ONLY)
configure_file(fdio.${CMAKE_SYSTEM_NAME}.h.in           include/config/fdio.h @ONLY)
configure_file(filelength.${CMAKE_SYSTEM_NAME}.h.in     include/config/filelength.h @ONLY)
configure_file(getpid.${CMAKE_SYSTEM_NAME}.h.in         include/config/getpid.h @ONLY)
configure_file(path_limits.${CMAKE_SYSTEM_NAME}.h.in    include/config/path_limits.h @ONLY)
configure_file(string_lower.${CMAKE_SYSTEM_NAME}.h.in   include/config/string_lower.h @ONLY)

add_library(config
    driver_types.h.in
    fdio.Linux.h.in fdio.Windows.h.in
    filelength.Linux.h.in filelength.Windows.h.in
    getpid.Linux.h.in getpid.Windows.h.in
    path_limits.Linux.h.in path_limits.Windows.h.in
    include/config/port.h
    port_config.h.in
    include/config/string_case_compare.h string_case_compare.cpp
    string_lower.Linux.h.in string_lower.Windows.h.in ${ID_STRING_LOWER_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/driver_types.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/fdio.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/filelength.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/getpid.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/path_limits.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/port_config.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/string_lower.h"
)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
set_target_properties(config PROPERTIES FOLDER Libraries)
target_include_directories(config PUBLIC
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")

add_subdirectory(tests)
