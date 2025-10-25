# SPDX-License-Identifier: GPL-3.0-only
#
include(CheckIncludeFile)
include(CheckSymbolExists)
include(TestBigEndian)

function(configure_flavor_header name flavor)
    configure_file(
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/${name}.${flavor}.h.in"
        "include/config/${name}.h"
        COPYONLY)
endfunction()

test_big_endian(ID_BIG_ENDIAN)
configure_file(port_config.h.in include/config/port_config.h)

check_include_file("fcntl.h"    HAVE_FCNTL_H)
check_include_file("io.h"       HAVE_IO_H)
check_include_file("process.h"  HAVE_PROCESS_H)
check_include_file("stdlib.h"   HAVE_STDLIB_H)
check_include_file("string.h"   HAVE_STRING_H)
check_include_file("strings.h"  HAVE_STRINGS_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("unistd.h"   HAVE_UNISTD_H)
check_include_file("Windows.h"  HAVE_WINDOWS_H)

# Determine available driver types
if(ID_USE_WX)
    set(ID_HAVE_WX_DRIVER           ON)
    set(ID_HAVE_WX_DISK_DRIVER      ON)
    set(ID_HAVE_CURSES_DRIVER       OFF)
    set(ID_HAVE_GDI_DRIVER          OFF)
    set(ID_HAVE_WIN32_DISK_DRIVER   OFF)
else()
    set(ID_HAVE_WX_DRIVER           OFF)
    set(ID_HAVE_WX_DISK_DRIVER      OFF)
    set(ID_HAVE_CURSES_DRIVER       OFF) # not yet implemented
    set(ID_HAVE_GDI_DRIVER          ${HAVE_WINDOWS_H})
    set(ID_HAVE_WIN32_DISK_DRIVER   ${HAVE_WINDOWS_H})
endif()
configure_file(driver_types.h.in include/config/driver_types.h @ONLY)

# Determine command shell flavor
if(NOT CMD_SHELL_FLAVOR)
    if(DEFINED ENV{COMSPEC})
        set(CMD_SHELL_DEFAULT_FLAVOR "comspec")
    elseif(DEFINED ENV{SHELL})
        set(CMD_SHELL_DEFAULT_FLAVOR "shell")
    else()
        set(CMD_SHELL_DEFAULT_FLAVOR "null")
    endif()
endif()
set(CMD_SHELL_FLAVOR "${CMD_SHELL_DEFAULT_FLAVOR}" CACHE STRING "The command-shell algorithm to use: null, comspec, shell")
configure_file("cmd_shell.${CMD_SHELL_FLAVOR}.cpp.in" "cmd_shell.cpp" COPYONLY)

# Determine file descriptor I/O flavor
if(HAVE_FCNTL_H)
    check_symbol_exists(open "fcntl.h" HAVE_FCNTL_OPEN)
    if(HAVE_FCNTL_OPEN)
        set(FDIO_FLAVOR "fcntl")
    endif()
endif()
if(NOT FDIO_FLAVOR AND HAVE_IO_H)
    check_symbol_exists(open "io.h" HAVE_IO_OPEN)
    if(HAVE_IO_OPEN)
        set(FDIO_FLAVOR "io")
    endif()
endif()
if(NOT FDIO_FLAVOR)
    message(FATAL_ERROR "Couldn't find open() in <fcntl.h> or <io.h>; file descriptor I/O not found.")
endif()
configure_flavor_header("fdio" "${FDIO_FLAVOR}")

# Determine filelength flavor
if(HAVE_IO_H)
    check_symbol_exists(filelength "io.h" HAVE_IO_FILELENGTH)
    if(HAVE_IO_FILELENGTH)
        set(FILELENGTH_FLAVOR "io")
    endif()
endif()
if(NOT FILELENGTH_FLAVOR AND HAVE_SYS_STAT_H)
    check_symbol_exists(fstat "sys/stat.h" HAVE_SYS_STAT_FSTAT)
    if(HAVE_SYS_STAT_FSTAT)
        set(FILELENGTH_FLAVOR "stat")
    endif()
endif()
if(NOT FILELENGTH_FLAVOR)
    message(FATAL_ERROR "Couldn't find fstat() in <sys/stat.h> or filelength() in <io.h>; filelength not found.")
endif()
configure_flavor_header("filelength" "${FILELENGTH_FLAVOR}")

# Determine string to lower-case flavor
if(HAVE_STRING_H)
    check_symbol_exists(strlwr "string.h" HAVE_STRLWR)
    if(HAVE_STRLWR)
        set(STRING_LOWER_FLAVOR "string")
        set(ID_STRING_LOWER_SOURCES "")
    endif()
endif()
if(NOT STRING_LOWER_FLAVOR)
    set(STRING_LOWER_FLAVOR "manual")
    set(ID_STRING_LOWER_SOURCES "string_lower.cpp")
endif()
configure_flavor_header("string_lower" "${STRING_LOWER_FLAVOR}")

# Determine getpid flavor
if(HAVE_UNISTD_H)
    check_symbol_exists(getpid "unistd.h" HAVE_UNISTD_GETPID)
    if(HAVE_UNISTD_GETPID)
        set(GETPID_FLAVOR "unistd")
    endif()
endif()
if(NOT GETPID_FLAVOR AND HAVE_PROCESS_H)
    check_symbol_exists(getpid "process.h" HAVE_PROCESS_GETPID)
    if(HAVE_PROCESS_GETPID)
        set(GETPID_FLAVOR "process")
    endif()
endif()
if(NOT GETPID_FLAVOR)
    message(FATAL_ERROR "Couldn't find getpid() in <unistd.h> or <process.h>.")
endif()
configure_flavor_header("getpid" "${GETPID_FLAVOR}")

# Determine path limits flavor
if(HAVE_STDLIB_H)
    check_symbol_exists(_MAX_PATH "stdlib.h" HAVE_STDLIB_MAX_PATH)
    if(HAVE_STDLIB_MAX_PATH)
        set(PATH_LIMITS_FLAVOR "stdlib")
    endif()
endif()
if(NOT PATH_LIMITS_FLAVOR)
    set(PATH_LIMITS_FLAVOR "manual")
endif()
configure_flavor_header("path_limits" "${PATH_LIMITS_FLAVOR}")

# Configure HOME_DIR
configure_file("home_dir.h.in" "include/config/home_dir.h" @ONLY)

add_library(config
    driver_types.h.in
    fdio.fcntl.h.in fdio.io.h.in
    filelength.io.h.in filelength.stat.h.in
    getpid.process.h.in getpid.unistd.h.in
    home_dir.h.in
    path_limits.manual.h.in path_limits.stdlib.h.in
    include/config/port.h
    port_config.h.in
    include/config/string_case_compare.h string_case_compare.cpp
    string_lower.string.h.in string_lower.manual.h.in ${ID_STRING_LOWER_SOURCES}
    cmd_shell.comspec.cpp.in cmd_shell.null.cpp.in cmd_shell.shell.cpp.in
    include/config/cmd_shell.h "${CMAKE_CURRENT_BINARY_DIR}/cmd_shell.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/driver_types.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/fdio.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/filelength.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/getpid.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/home_dir.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/path_limits.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/port_config.h"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config/string_lower.h"
)
source_group("CMake Templates" REGULAR_EXPRESSION "^.*\.in$")
target_folder(config "Libraries")
target_include_directories(config PUBLIC
    include
    "${CMAKE_CURRENT_BINARY_DIR}/include")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
