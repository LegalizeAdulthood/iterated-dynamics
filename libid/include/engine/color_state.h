// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class ColorState
{
    DEFAULT_MAP = 0, // g_dac_box matches default map
    UNKNOWN_MAP = 1, // g_dac_box matches no known defined map
    MAP_FILE = 2,    // g_dac_box matches the color file map
};

extern ColorState g_color_state;
