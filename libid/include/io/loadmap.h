// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <string>

// returns true on error
bool validate_luts(const std::string &map_name);
void set_color_palette_name(const std::string &filename);
