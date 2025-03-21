# SPDX-License-Identifier: GPL-3.0-only
#
find_package(GIF REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(image-tool STATIC
    GIFInputFile.h GIFInputFile.cpp
    gif_compare.h
    gif_format.h
    gif_json.h
)
target_include_directories(image-tool PUBLIC .)
target_link_libraries(image-tool PUBLIC GIF::GIF fmt::fmt-header-only)
target_folder(image-tool "Tests/Libraries")

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC image-tool)
target_folder(image-compare "Tests/Tools")

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC image-tool)
target_folder(image-dump "Tests/Tools")
