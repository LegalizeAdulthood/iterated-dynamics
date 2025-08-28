# SPDX-License-Identifier: GPL-3.0-only
#
# Copyright 2025 Richard Thomson
#
if(NOT CPACK_WIX_ROOT)
    string(REPLACE "\\" "/" CPACK_WIX_ROOT "$ENV{WIX}")
endif()

find_program(CPACK_WIX_CANDLE_EXECUTABLE candle
    PATHS "${CPACK_WIX_ROOT}" PATH_SUFFIXES "bin")

find_program(CPACK_WIX_LIGHT_EXECUTABLE light
    PATHS "${CPACK_WIX_ROOT}" PATH_SUFFIXES "bin")
