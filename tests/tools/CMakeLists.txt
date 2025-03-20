# SPDX-License-Identifier: GPL-3.0-only
#
find_package(GIF REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC GIF::GIF)
set_target_properties(image-compare PROPERTIES FOLDER Tests)

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC GIF::GIF fmt::fmt-header-only)
set_target_properties(image-dump PROPERTIES FOLDER Tests)
