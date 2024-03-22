#pragma once

extern double              (*g_dx_pixel)();
extern double              (*g_dy_pixel)();
extern long                (*g_l_x_pixel)();
extern long                (*g_l_y_pixel)();

void set_pixel_calc_functions();
