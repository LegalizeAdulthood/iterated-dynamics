// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <string>

namespace id::io
{

extern std::string           g_last_map_name;       // from last <l> <s> or colors=@filename
extern Byte                  g_map_clut[256][3];    // map= (default colors)
extern bool                  g_map_specified;       // map= specified

struct PaletteLoadResult
{
    bool failed{};
    std::string diagnostic;
};

PaletteLoadResult validate_luts(const std::string &map_name);
PaletteLoadResult set_color_palette_name(const std::string &map_name);

} // namespace id::io
