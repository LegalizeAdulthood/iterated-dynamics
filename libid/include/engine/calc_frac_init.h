// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <array>

namespace id::engine
{

constexpr float DEFAULT_ASPECT{0.75F};          // Assumed overall screen dimensions, y/x
constexpr float DEFAULT_ASPECT_DRIFT{0.02F};    // drift of < 2% is forced to 0%

extern float                 g_aspect_drift;    // how much drift is allowed and still forced to g_screen_aspect
extern float                 g_screen_aspect;   // aspect ratio of the screen
extern double                g_delta_min;       // same as a double
extern LDouble               g_delta_x2;        // screen pixel increments
extern LDouble               g_delta_x;
extern LDouble               g_delta_y2;
extern LDouble               g_delta_y;
extern std::array<double, 2> g_math_tol;        // For math transition from double to bignum
extern double                g_plot_mx1;        // real->screen multipliers
extern double                g_plot_mx2;
extern double                g_plot_my1;
extern double                g_plot_my2;

void calc_frac_init();
void adjust_corner();
void adjust_corner_bf();
void fractal_float_to_bf();

} // namespace id::engine
