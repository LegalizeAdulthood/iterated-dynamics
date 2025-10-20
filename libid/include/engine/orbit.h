// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

extern int g_orbit_delay; // microsecond orbit delay

void plot_orbit(double real, double imag, int color);
void scrub_orbit();

} // namespace id::engine
