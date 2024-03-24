#pragma once

#include <vector>

extern std::vector<double>   g_grid_x0;
extern std::vector<double>   g_grid_x1;
extern std::vector<double>   g_grid_y0;
extern std::vector<double>   g_grid_y1;
extern std::vector<long>     g_l_x0;
extern std::vector<long>     g_l_x1;
extern std::vector<long>     g_l_y0;
extern std::vector<long>     g_l_y1;
extern double              (*g_dx_pixel)();
extern double              (*g_dy_pixel)();
extern long                (*g_l_x_pixel)();
extern long                (*g_l_y_pixel)();

void set_pixel_calc_functions();

void set_grid_pointers();
void free_grid_pointers();

void fill_dx_array();
void fill_lx_array();
