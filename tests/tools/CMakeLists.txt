# SPDX-License-Identifier: GPL-3.0-only
#
find_package(GIF REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC GIF::GIF)
target_folder(image-compare "Tests")

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC GIF::GIF fmt::fmt-header-only)
target_folder(image-dump "Tests")
