# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
# Usage:
#   cmake -DRELEASE_VERSION:STRING=1.4.0 -DNEXT_VERSION:STRING=1.4.1 -P scripts/release-published.cmake
#
# Variables:
#   RELEASE_VERSION Version that was just published, with optional leading v.
#   NEXT_VERSION    Next development version.
#   ID_SOURCE_DIR   Source tree to update; defaults to this script's parent.
#   RELEASE_PAGES_DIR
#                   Optional gh-pages checkout whose current docs are promoted.
#   RELEASE_DOC_VERSION
#                   Optional versioned docs directory under RELEASE_PAGES_DIR;
#                   defaults to vM.N.P from RELEASE_VERSION.
#   DRY_RUN         If true, validate and report writes without changing files.
#
cmake_minimum_required(VERSION 3.23)

set(ID_RELEASE_BACKSLASH_TOKEN "@@ID_RELEASE_BACKSLASH@@")

function(id_append_line lines_var line)
    set(id_line "${line}")
    string(REPLACE "\\" "${ID_RELEASE_BACKSLASH_TOKEN}" id_line "${id_line}")
    string(REPLACE ";" "\\;" id_line "${id_line}")
    set(lines "${${lines_var}}")
    list(APPEND lines "${id_line}")
    set(${lines_var} "${lines}" PARENT_SCOPE)
endfunction()

function(id_unprotect_line line out_var)
    string(REPLACE "${ID_RELEASE_BACKSLASH_TOKEN}" "\\" result "${line}")
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
    set(${prefix}_TAG "v${major}.${minor}.${patch}" PARENT_SCOPE)
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

function(id_find_line lines_var wanted path out_index)
    set(match_count 0)
    set(match_index -1)
    list(LENGTH ${lines_var} line_count)
    if(line_count GREATER 0)
        math(EXPR last_index "${line_count} - 1")
        foreach(index RANGE 0 ${last_index})
            list(GET ${lines_var} ${index} line)
            id_unprotect_line("${line}" line)
            if(line STREQUAL "${wanted}")
                math(EXPR match_count "${match_count} + 1")
                set(match_index "${index}")
            endif()
        endforeach()
    endif()

    if(NOT match_count EQUAL 1)
        id_fail("${path} must contain exactly one ${wanted} line")
    endif()
    set(${out_index} "${match_index}" PARENT_SCOPE)
endfunction()

function(id_insert_lines lines_var index insert_var out_var)
    set(result)
    list(LENGTH ${lines_var} line_count)
    foreach(line_index RANGE 0 ${line_count})
        if(line_index EQUAL index)
            foreach(line IN LISTS ${insert_var})
                id_append_line(result "${line}")
            endforeach()
        endif()
        if(line_index LESS line_count)
            list(GET ${lines_var} ${line_index} line)
            id_append_line(result "${line}")
        endif()
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(id_replace_lines lines_var first_index last_index replacement_var out_var)
    set(result)
    list(LENGTH ${lines_var} line_count)
    if(line_count GREATER 0)
        math(EXPR final_index "${line_count} - 1")
        foreach(line_index RANGE 0 ${final_index})
            if(line_index EQUAL first_index)
                foreach(line IN LISTS ${replacement_var})
                    id_append_line(result "${line}")
                endforeach()
            endif()
            if(line_index LESS first_index OR line_index GREATER last_index)
                list(GET ${lines_var} ${line_index} line)
                id_append_line(result "${line}")
            endif()
        endforeach()
    endif()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(id_replace_text lines_var old_text new_text out_var)
    set(result)
    foreach(line IN LISTS ${lines_var})
        id_unprotect_line("${line}" line)
        string(REPLACE "${old_text}" "${new_text}" line "${line}")
        id_append_line(result "${line}")
    endforeach()
    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(id_make_current_changelog next_version out_var)
    set(lines)
    id_append_line(lines "; RELEASE CHANGELOG BEGIN")
    id_append_line(lines "~Topic=New Features in ${next_version}")
    id_append_line(lines "")
    id_append_line(lines "The source code for Iterated Dynamics lives on github:\\")
    id_append_line(lines "  <https://github.com/LegalizeAdulthood/iterated-dynamics>")
    id_append_line(lines "")
    id_append_line(lines "Found a bug?  File an issue on our github project!\\")
    id_append_line(lines "  <https://github.com/LegalizeAdulthood/iterated-dynamics/issues>")
    id_append_line(lines "")
    id_append_line(lines "Version ${next_version} is an interim release on the way to version 2.0.")
    id_append_line(lines "")
    id_append_line(lines "New Features in Version ${next_version}")
    id_append_line(lines "")
    id_append_line(lines "Bugs Fixed in Version ${next_version}")
    id_append_line(lines "")
    id_append_line(lines "Code Cleanup in Version ${next_version}")
    id_append_line(lines ";")
    id_append_line(lines ";")
    id_append_line(lines ";")
    id_append_line(lines "; RELEASE CHANGELOG END")
    set(${out_var} "${lines}" PARENT_SCOPE)
endfunction()

function(id_extract_changelog lines_var release_version out_var)
    id_find_line(${lines_var} "; RELEASE CHANGELOG BEGIN" "hc/src/help.src" begin_index)
    id_find_line(${lines_var} "; RELEASE CHANGELOG END" "hc/src/help.src" end_index)
    if(NOT begin_index LESS end_index)
        id_fail("RELEASE CHANGELOG BEGIN must precede RELEASE CHANGELOG END")
    endif()

    set(history_start -1)
    math(EXPR first_index "${begin_index} + 1")
    math(EXPR last_index "${end_index} - 1")
    foreach(line_index RANGE ${first_index} ${last_index})
        list(GET ${lines_var} ${line_index} line)
        id_unprotect_line("${line}" line)
        if(line MATCHES "^Version ${release_version}([ .]|$)")
            set(history_start "${line_index}")
            break()
        endif()
    endforeach()
    if(history_start EQUAL -1)
        id_fail("Could not find Version ${release_version} in changelog")
    endif()

    set(history_end "${last_index}")
    while(history_end GREATER_EQUAL history_start)
        list(GET ${lines_var} ${history_end} line)
        id_unprotect_line("${line}" line)
        if(line STREQUAL "" OR line STREQUAL ";")
            math(EXPR history_end "${history_end} - 1")
        else()
            break()
        endif()
    endwhile()

    set(history)
    foreach(line_index RANGE ${history_start} ${history_end})
        list(GET ${lines_var} ${line_index} line)
        id_unprotect_line("${line}" line)
        id_append_line(history "${line}")
    endforeach()
    set(${out_var} "${history}" PARENT_SCOPE)
endfunction()

function(id_update_changelog help_src_path help5_src_path release_version next_version)
    id_read_lines("${help_src_path}" help_src_lines)
    id_read_lines("${help5_src_path}" help5_lines)

    id_extract_changelog(help_src_lines "${release_version}" history_lines)
    foreach(line IN LISTS help5_lines)
        id_unprotect_line("${line}" line)
        if(line MATCHES "^Version ${release_version}([ .]|$)")
            id_fail("Version ${release_version} is already in revision history")
        endif()
    endforeach()

    id_find_line(help5_lines "; RELEASE HISTORY INSERT" "hc/src/help5.src" marker_index)
    math(EXPR insert_index "${marker_index} + 1")

    set(history_insert "${history_lines}")
    id_append_line(history_insert "")
    id_insert_lines(help5_lines "${insert_index}" history_insert help5_lines)
    id_write_lines("${help5_src_path}" help5_lines)

    id_make_current_changelog("${next_version}" current_changelog)
    id_find_line(help_src_lines "; RELEASE CHANGELOG BEGIN" "hc/src/help.src" begin_index)
    id_find_line(help_src_lines "; RELEASE CHANGELOG END" "hc/src/help.src" end_index)
    id_replace_lines(help_src_lines "${begin_index}" "${end_index}" current_changelog help_src_lines)
    id_replace_text(
        help_src_lines
        "New Features in ${release_version}"
        "New Features in ${next_version}"
        help_src_lines)
    id_write_lines("${help_src_path}" help_src_lines)
endfunction()

function(id_update_source_version source_dir current_version next_version)
    set(args
        "${CMAKE_COMMAND}"
        "-DID_SOURCE_DIR:STRING=${source_dir}"
        "-DCURRENT_VERSION:STRING=${current_version}"
        "-DNEXT_VERSION:STRING=${next_version}")
    if(DRY_RUN)
        list(APPEND args "-DDRY_RUN:BOOL=ON")
    endif()
    list(APPEND args -P "${CMAKE_CURRENT_LIST_DIR}/new-version.cmake")

    execute_process(COMMAND ${args} RESULT_VARIABLE result)
    if(NOT result EQUAL 0)
        id_fail("new-version.cmake failed with result ${result}")
    endif()
endfunction()

function(id_promote_docs pages_dir doc_version)
    if(NOT IS_DIRECTORY "${pages_dir}")
        id_fail("Missing pages directory: ${pages_dir}")
    endif()

    set(versioned_dir "${pages_dir}/${doc_version}")
    set(versioned_index "${versioned_dir}/index.html")
    if(NOT EXISTS "${versioned_index}")
        id_fail("Missing versioned documentation: ${versioned_index}")
    endif()

    if(DRY_RUN)
        message(STATUS "Would promote ${versioned_dir} to ${pages_dir}")
        return()
    endif()

    file(COPY_FILE "${versioned_index}" "${pages_dir}/index.html")
    file(REMOVE_RECURSE "${pages_dir}/help")
    if(IS_DIRECTORY "${versioned_dir}/help")
        file(COPY "${versioned_dir}/help" DESTINATION "${pages_dir}")
    endif()
endfunction()

id_require_var(RELEASE_VERSION)
id_require_var(NEXT_VERSION)
id_parse_version("${RELEASE_VERSION}" RELEASE)
id_parse_version("${NEXT_VERSION}" NEXT)

if(NOT DEFINED ID_SOURCE_DIR OR "${ID_SOURCE_DIR}" STREQUAL "")
    get_filename_component(ID_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()
file(TO_CMAKE_PATH "${ID_SOURCE_DIR}" ID_SOURCE_DIR)

id_update_source_version(
    "${ID_SOURCE_DIR}"
    "${RELEASE_PROJECT}"
    "${NEXT_PROJECT}")
id_update_changelog(
    "${ID_SOURCE_DIR}/hc/src/help.src"
    "${ID_SOURCE_DIR}/hc/src/help5.src"
    "${RELEASE_DISPLAY}"
    "${NEXT_DISPLAY}")

if(DEFINED RELEASE_PAGES_DIR AND NOT "${RELEASE_PAGES_DIR}" STREQUAL "")
    if(NOT DEFINED RELEASE_DOC_VERSION OR "${RELEASE_DOC_VERSION}" STREQUAL "")
        set(RELEASE_DOC_VERSION "${RELEASE_TAG}")
    endif()
    id_promote_docs("${RELEASE_PAGES_DIR}" "${RELEASE_DOC_VERSION}")
endif()

message(STATUS "Prepared post-release source updates for ${NEXT_DISPLAY}")
