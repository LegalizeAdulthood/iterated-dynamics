# SPDX-License-Identifier: GPL-3.0-only
#
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO LegalizeAdulthood/tgautils
    REF 9e03533c4a35341deddfb5394d2f9faf8afa5402
    SHA512 a349923d4ea4b9a82c412e6925f37fac00f43a8b98e1191b1fb192bb1640bee1e1f6f4d89f6aba70fdcf7ead500b9680eac0407938f353634853da4bc37fdc86
    HEAD_REF main
)

file(WRITE "${SOURCE_PATH}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.23)
project(tgautils C)

add_library(tga STATIC
    libs/read.c
    libs/write.c
)
target_include_directories(tga PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/libs/include>"
    "$<INSTALL_INTERFACE:include>"
)

install(TARGETS tga
    EXPORT tgautilsTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(FILES libs/include/tga.h DESTINATION include)
install(EXPORT tgautilsTargets
    FILE tgautilsTargets.cmake
    NAMESPACE tgautils::
    DESTINATION share/tgautils
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/tgautilsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tgautilsConfig.cmake"
    INSTALL_DESTINATION share/tgautils
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tgautilsConfig.cmake"
    DESTINATION share/tgautils
)
]=])

file(WRITE "${SOURCE_PATH}/tgautilsConfig.cmake.in" [=[
@PACKAGE_INIT@

include("${CMAKE_CURRENT_LIST_DIR}/tgautilsTargets.cmake")
]=])

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH share/tgautils)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
