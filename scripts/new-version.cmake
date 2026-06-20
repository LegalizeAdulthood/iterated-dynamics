# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
# Usage:
#   cmake -DCURRENT_VERSION:STRING=1.4.1 -DNEXT_VERSION:STRING=1.5.0 -P scripts/new-version.cmake
#
# Variables:
#   CURRENT_VERSION Current source version, with optional leading v.
#   NEXT_VERSION    New source version.
#   ID_SOURCE_DIR   Source tree to update; defaults to this script's parent.
#   DRY_RUN         If true, validate and report writes without changing files.
#
cmake_minimum_required(VERSION 3.23)

set(ID_NEW_VERSION_BACKSLASH_TOKEN "@@ID_NEW_VERSION_BACKSLASH@@")

function(id_append_line lines_var line)
    set(id_line "${line}")
    string(REPLACE "\\" "${ID_NEW_VERSION_BACKSLASH_TOKEN}" id_line "${id_line}")
    string(REPLACE ";" "\\;" id_line "${id_line}")
    set(lines "${${lines_var}}")
    list(APPEND lines "${id_line}")
    set(${lines_var} "${lines}" PARENT_SCOPE)
endfunction()

function(id_unprotect_line line out_var)
    string(REPLACE "${ID_NEW_VERSION_BACKSLASH_TOKEN}" "\\" result "${line}")
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(id_fail message_text)
    message(FATAL_ERROR "${message_text}")
endfunction()

function(id_require_var name)
    if(NOT DEFINED ${name} OR "${${name}}" STREQUAL "")
        id_fail("${name} must be specified")
    endif()
endfunction()

function(id_parse_version value prefix)
    if(NOT "${value}" MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)(\\.([0-9]+))?$")
        id_fail("Invalid version: ${value}")
    endif()

    set(major "${CMAKE_MATCH_1}")
    set(minor "${CMAKE_MATCH_2}")
    set(patch "${CMAKE_MATCH_3}")
    set(tweak "${CMAKE_MATCH_5}")
    if("${tweak}" STREQUAL "")
        set(tweak 0)
    endif()

    set(${prefix}_DISPLAY "${major}.${minor}.${patch}" PARENT_SCOPE)
    set(${prefix}_PROJECT "${major}.${minor}.${patch}.${tweak}" PARENT_SCOPE)
endfunction()

function(id_read_lines path out_var)
    if(NOT EXISTS "${path}")
        id_fail("Missing file: ${path}")
    endif()
    file(READ "${path}" content)
    set(lines)
    while(TRUE)
        string(FIND "${content}" "\n" newline)
        if(newline EQUAL -1)
            set(line "${content}")
            if(NOT line STREQUAL "")
                string(REGEX REPLACE "\r$" "" line "${line}")
                id_append_line(lines "${line}")
            endif()
            break()
        endif()

        string(SUBSTRING "${content}" 0 ${newline} line)
        math(EXPR next_line "${newline} + 1")
        string(SUBSTRING "${content}" ${next_line} -1 content)
        string(REGEX REPLACE "\r$" "" line "${line}")
        id_append_line(lines "${line}")
    endwhile()
    set(${out_var} "${lines}" PARENT_SCOPE)
endfunction()

function(id_write_lines path lines_var)
    if(DRY_RUN)
        message(STATUS "Would write ${path}")
        return()
    endif()

    file(WRITE "${path}" "")
    foreach(line IN LISTS ${lines_var})
        id_unprotect_line("${line}" line)
        file(APPEND "${path}" "${line}\n")
    endforeach()
endfunction()

function(id_update_project_version path current_project next_project)
    id_read_lines("${path}" lines)
    set(result)
    set(match_count 0)
    list(LENGTH lines line_count)
    if(line_count GREATER 0)
        math(EXPR last_index "${line_count} - 1")
        foreach(line_index RANGE 0 ${last_index})
            list(GET lines ${line_index} line)
            id_unprotect_line("${line}" line)
            if(line MATCHES "^([ \t]*)VERSION[ \t]+${current_project}[ \t]*$")
                id_append_line(result "${CMAKE_MATCH_1}VERSION ${next_project}")
                math(EXPR match_count "${match_count} + 1")
            else()
                id_append_line(result "${line}")
            endif()
        endforeach()
    endif()
    if(NOT match_count EQUAL 1)
        id_fail("CMake project version is not ${current_project}")
    endif()
    id_write_lines("${path}" result)
endfunction()

function(id_update_product_guid path next_version)
    string(UUID product_guid
        NAMESPACE "0E9EE50A-71DB-4723-BC37-CC831B7A7EF7"
        NAME "Iterated Dynamics ${next_version}"
        TYPE SHA1
        UPPER)

    id_read_lines("${path}" lines)
    set(result)
    set(match_count 0)
    list(LENGTH lines line_count)
    if(line_count GREATER 0)
        math(EXPR last_index "${line_count} - 1")
        foreach(line_index RANGE 0 ${last_index})
            list(GET lines ${line_index} line)
            id_unprotect_line("${line}" line)
            if(line MATCHES "^([ \t]*set\\(CPACK_WIX_PRODUCT_GUID[ \t]+)\"[0-9A-Fa-f-]+\"(\\).*)$")
                set(line "${CMAKE_MATCH_1}\"${product_guid}\"${CMAKE_MATCH_2}")
                math(EXPR match_count "${match_count} + 1")
            endif()
            id_append_line(result "${line}")
        endforeach()
    endif()
    if(NOT match_count EQUAL 1)
        id_fail("Could not find CPACK_WIX_PRODUCT_GUID assignment")
    endif()
    id_write_lines("${path}" result)
endfunction()

id_require_var(CURRENT_VERSION)
id_require_var(NEXT_VERSION)
id_parse_version("${CURRENT_VERSION}" CURRENT)
id_parse_version("${NEXT_VERSION}" NEXT)

if(NOT DEFINED ID_SOURCE_DIR OR "${ID_SOURCE_DIR}" STREQUAL "")
    get_filename_component(ID_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
file(TO_CMAKE_PATH "${ID_SOURCE_DIR}" ID_SOURCE_DIR)

id_update_project_version(
    "${ID_SOURCE_DIR}/CMakeLists.txt"
    "${CURRENT_PROJECT}"
    "${NEXT_PROJECT}")
id_update_product_guid(
    "${ID_SOURCE_DIR}/packaging/CMakeLists.txt"
    "${NEXT_DISPLAY}")

message(STATUS "Updated source version from ${CURRENT_DISPLAY} to ${NEXT_DISPLAY}")
