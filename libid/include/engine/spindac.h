// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern int g_dac_count;
extern bool g_is_true_color;

enum class SpinDirection
{
    NONE = 0,
    FORWARD = 1,
    BACKWARD = -1,
};

inline int operator+(const SpinDirection value)
{
    return static_cast<int>(value);
}

void spin_dac(SpinDirection dir, int inc);

/// Updates the colormap without spinning the colors
inline void refresh_dac()
{
    spin_dac(SpinDirection::NONE, 1);
}

} // namespace id::engine
