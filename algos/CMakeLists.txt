# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2026 Richard Thomson
#
add_library(algos STATIC
    include/algos/string_algorithms.h
    string_algorithms.cpp)
target_compile_definitions(algos PUBLIC ${ID_TARGET_DEFINITIONS} $<$<CONFIG:Debug>:${ID_TARGET_DEFINITIONS_DEBUG}>)
target_compile_options(algos PRIVATE ${ID_TARGET_OPTIONS})
target_include_directories(algos PUBLIC include)
target_folder(algos "Libraries")
