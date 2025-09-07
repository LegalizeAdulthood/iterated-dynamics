// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <vector>

namespace id
{

extern bool                  g_use_grid;
extern std::vector<double>   g_grid_x0;
extern std::vector<double>   g_grid_x1;
extern std::vector<double>   g_grid_y0;
extern std::vector<double>   g_grid_y1;
extern double              (*g_dx_pixel)();
extern double              (*g_dy_pixel)();

void set_pixel_calc_functions();

void set_grid_pointers();
void free_grid_pointers();

void fill_dx_array();

} // namespace id
