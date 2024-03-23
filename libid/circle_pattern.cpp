#include "circle_pattern.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"

int CirclefpFractal()
{
    long i;
    i = (long)(g_params[0]*(g_temp_sqr_x+g_temp_sqr_y));
    g_color_iter = i%g_colors;
    return 1;
}
/*
CirclelongFractal()
{
   long i;
   i = multiply(lparm.x,(g_l_temp_sqr_x+g_l_temp_sqr_y),g_bit_shift);
   i = i >> g_bit_shift;
   g_color_iter = i%colors);
   return 1;
}
*/
