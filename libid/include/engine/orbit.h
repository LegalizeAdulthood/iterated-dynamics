// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern int g_orbit_delay;       // 100-usec orbit playback delay
extern int g_orbit_skip_points; // initial orbit points skipped by passes=o

void plot_orbit(double real, double imag, int color);
void scrub_orbit();

} // namespace id::engine
