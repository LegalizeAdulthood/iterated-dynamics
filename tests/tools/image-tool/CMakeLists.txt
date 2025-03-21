# SPDX-License-Identifier: GPL-3.0-only
#
find_package(GIF REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_library(image-tool STATIC
    include/image-tool/GIFInputFile.h GIFInputFile.cpp
    include/image-tool/gif_compare.h
    include/image-tool/gif_format.h
    include/image-tool/gif_json.h
)
target_include_directories(image-tool PUBLIC include)
target_link_libraries(image-tool PUBLIC GIF::GIF fmt::fmt-header-only)
target_folder(image-tool "Tests/Libraries")
