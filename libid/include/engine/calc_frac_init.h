// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::engine
{

extern double                g_delta_min;
extern LDouble               g_delta_x2;
extern LDouble               g_delta_x;
extern LDouble               g_delta_y2;
extern LDouble               g_delta_y;
extern double                g_plot_mx1;
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;

void calc_frac_init();
void adjust_corner();
void adjust_corner_bf();
void fractal_float_to_bf();

} // namespace id::engine
