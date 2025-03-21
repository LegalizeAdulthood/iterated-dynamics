# SPDX-License-Identifier: GPL-3.0-only
#
add_subdirectory(image-tool)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC image-tool)
target_folder(image-compare "Tests/Tools")

add_executable(image-dump image_dump.cpp)
target_link_libraries(image-dump PUBLIC image-tool)
target_folder(image-dump "Tests/Tools")
