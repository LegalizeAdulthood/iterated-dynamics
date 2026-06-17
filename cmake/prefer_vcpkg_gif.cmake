# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
# FindGIF reads GIF_DIR as an environment hint for both the header and library.
#

function(_prefer_vcpkg_gif_find_root result)
    set(${result} "" PARENT_SCOPE)
    if(NOT DEFINED VCPKG_INSTALLED_DIR)
        return()
    endif()
    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(VCPKG_GIF_ROOT
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
        if(EXISTS "${VCPKG_GIF_ROOT}/include/gif_lib.h")
            set(${result} "${VCPKG_GIF_ROOT}" PARENT_SCOPE)
            return()
        endif()
    endif()
    file(GLOB VCPKG_GIF_HEADERS
        LIST_DIRECTORIES false
        "${VCPKG_INSTALLED_DIR}/*/include/gif_lib.h")
    list(LENGTH VCPKG_GIF_HEADERS VCPKG_GIF_HEADER_COUNT)
    if(VCPKG_GIF_HEADER_COUNT EQUAL 1)
        list(GET VCPKG_GIF_HEADERS 0 VCPKG_GIF_HEADER)
        get_filename_component(VCPKG_GIF_INCLUDE_DIR
            "${VCPKG_GIF_HEADER}" DIRECTORY)
        get_filename_component(VCPKG_GIF_ROOT
            "${VCPKG_GIF_INCLUDE_DIR}" DIRECTORY)
        set(${result} "${VCPKG_GIF_ROOT}" PARENT_SCOPE)
    endif()
endfunction()

function(_prefer_vcpkg_gif_check_root root)
    file(TO_CMAKE_PATH "${root}/include" VCPKG_GIF_INCLUDE_DIR)
    file(TO_CMAKE_PATH "${GIF_INCLUDE_DIR}" FOUND_GIF_INCLUDE_DIR)
    if(NOT FOUND_GIF_INCLUDE_DIR STREQUAL VCPKG_GIF_INCLUDE_DIR)
        message(FATAL_ERROR
            "FindGIF found GIF_INCLUDE_DIR=${GIF_INCLUDE_DIR}; "
            "expected ${VCPKG_GIF_INCLUDE_DIR}")
    endif()
endfunction()

function(_prefer_vcpkg_gif_configure_target)
    if(TARGET GIF::GIF)
        # Imported target includes are system includes by default, which lets
        # Homebrew's implicit include path win over vcpkg on macOS.
        set_target_properties(GIF::GIF PROPERTIES IMPORTED_NO_SYSTEM TRUE)
    endif()
endfunction()

function(prefer_vcpkg_gif)
    if(TARGET GIF::GIF)
        _prefer_vcpkg_gif_configure_target()
        return()
    endif()
    _prefer_vcpkg_gif_find_root(VCPKG_GIF_ROOT)
    if(VCPKG_GIF_ROOT)
        set(ENV{GIF_DIR} "${VCPKG_GIF_ROOT}")
        message(STATUS "Using FindGIF GIF_DIR hint: ${VCPKG_GIF_ROOT}")
    endif()
    find_package(GIF REQUIRED)
    _prefer_vcpkg_gif_configure_target()
    if(VCPKG_GIF_ROOT)
        _prefer_vcpkg_gif_check_root("${VCPKG_GIF_ROOT}")
    endif()
endfunction()
