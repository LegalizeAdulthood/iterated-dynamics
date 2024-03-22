#include "pixel_grid.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "fracsubr.h"
#include "id_data.h"

/*
 * The following functions calculate the real and imaginary complex
 * coordinates of the point in the complex plane corresponding to
 * the screen coordinates (col,row) at the current zoom corners
 * settings. The functions come in two flavors. One looks up the pixel
 * values using the precalculated grid arrays dx0, dx1, dy0, and dy1,
 * which has a speed advantage but is limited to MAX_PIXELS image
 * dimensions. The other calculates the complex coordinates at a
 * cost of two additions and two multiplications for each component,
 * but works at any resolution.
 *
 * The function call overhead
 * appears to be negligible. It also appears that the speed advantage
 * of the lookup vs the calculation is negligible on machines with
 * coprocessors. Bert Tyler's original implementation was designed for
 * machines with no coprocessor; on those machines the saving was
 * significant. For the time being, the table lookup capability will
 * be maintained.
 */

// Real component, grid lookup version - requires dx0/dx1 arrays
static double dxpixel_grid()
{
    return g_grid_x0[g_col]+g_grid_x1[g_row];
}

// Real component, calculation version - does not require arrays
static double dxpixel_calc()
{
    return (double)(g_x_min + g_col*g_delta_x + g_row*g_delta_x2);
}

// Imaginary component, grid lookup version - requires dy0/dy1 arrays
static double dypixel_grid()
{
    return g_grid_y0[g_row]+g_grid_y1[g_col];
}

// Imaginary component, calculation version - does not require arrays
static double dypixel_calc()
{
    return (double)(g_y_max - g_row*g_delta_y - g_col*g_delta_y2);
}

// Real component, grid lookup version - requires lx0/lx1 arrays
static long lxpixel_grid()
{
    return g_l_x0[g_col]+g_l_x1[g_row];
}

// Real component, calculation version - does not require arrays
static long lxpixel_calc()
{
    return g_l_x_min + g_col*g_l_delta_x + g_row*g_l_delta_x2;
}

// Imaginary component, grid lookup version - requires ly0/ly1 arrays
static long lypixel_grid()
{
    return g_l_y0[g_row]+g_l_y1[g_col];
}

// Imaginary component, calculation version - does not require arrays
static long lypixel_calc()
{
    return g_l_y_max - g_row*g_l_delta_y - g_col*g_l_delta_y2;
}

double (*g_dx_pixel)() = dxpixel_calc;
double (*g_dy_pixel)() = dypixel_calc;
long (*g_l_x_pixel)() = lxpixel_calc;
long (*g_l_y_pixel)() = lypixel_calc;

void set_pixel_calc_functions()
{
    if (g_use_grid)
    {
        g_dx_pixel = dxpixel_grid;
        g_dy_pixel = dypixel_grid;
        g_l_x_pixel = lxpixel_grid;
        g_l_y_pixel = lypixel_grid;
    }
    else
    {
        g_dx_pixel = dxpixel_calc;
        g_dy_pixel = dypixel_calc;
        g_l_x_pixel = lxpixel_calc;
        g_l_y_pixel = lypixel_calc;
    }
}
