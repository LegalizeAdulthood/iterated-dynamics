# SPDX-License-Identifier: GPL-3.0-only
#
find_package(GIF REQUIRED)

add_executable(image-compare image_compare.cpp)
target_link_libraries(image-compare PUBLIC GIF::GIF)
set_target_properties(image-compare PROPERTIES FOLDER Tests)
