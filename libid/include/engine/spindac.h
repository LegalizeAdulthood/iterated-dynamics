// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern int g_dac_count;
extern bool g_is_true_color;

void spin_dac(int dir, int inc);

/// Updates the colormap without spinning the colors
inline void refresh_dac()
{
    spin_dac(0, 1);
}

} // namespace id::engine
