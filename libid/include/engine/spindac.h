// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <string>

namespace id::engine
{

extern int g_color_cycle_range_lo; // cycling color range
extern int g_color_cycle_range_hi; //
extern int g_dac_count;            //
extern bool g_is_true_color;       //
extern Byte g_dac_box[256][3];     //
extern bool g_got_real_dac;        // load_dac worked, really got a dac
extern Byte g_old_dac_box[256][3]; //
extern std::string g_map_name;     //
extern bool g_map_set;             //

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
