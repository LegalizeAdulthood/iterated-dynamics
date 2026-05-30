# SPDX-License-Identifier: GPL-3.0-only
#
find_package(fmt CONFIG REQUIRED)
find_package(PNG REQUIRED)
find_package(tgautils CONFIG REQUIRED)

add_subdirectory(image-tool)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC image-tool fmt::fmt-header-only PNG::PNG tgautils::tga)
target_folder(image-compare "Tests/Tools")

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC image-tool)
target_folder(image-dump "Tests/Tools")

add_executable(colormap-compare colormap_compare.cpp)
target_link_libraries(colormap-compare PUBLIC image-tool)
target_folder(colormap-compare "Tests/Tools")

if(WIN32)
    add_executable(dump-dpi dump_dpi.cpp)
    target_link_libraries(dump-dpi "Shcore.lib" "User32.lib")
    target_folder(dump-dpi "Tests/Tools")
endif()
