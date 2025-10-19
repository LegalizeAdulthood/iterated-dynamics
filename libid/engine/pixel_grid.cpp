// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/pixel_grid.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"

namespace id::engine
{

// note that integer grid is set when integerfractal && !invert;
// otherwise the floating point grid is set; never both at once
// note that lx1 & ly1 values can overflow into sign bit; since
// they're used only to add to lx0/ly0, 2s comp straightens it out
static std::vector<double> s_grid_x0;            // floating pt equivs
static std::vector<double> s_grid_y0;
static std::vector<double> s_grid_x1;
static std::vector<double> s_grid_y1;

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
double dx_pixel()
{
    return s_grid_x0[g_col]+s_grid_x1[g_row];
}

// Imaginary component, grid lookup version - requires dy0/dy1 arrays
double dy_pixel()
{
    return s_grid_y0[g_row]+s_grid_y1[g_col];
}

void alloc_pixel_grid()
{
    free_pixel_grid();
    s_grid_x0.resize(g_logical_screen.x_dots);
    s_grid_y1.resize(g_logical_screen.x_dots);
    s_grid_y0.resize(g_logical_screen.y_dots);
    s_grid_x1.resize(g_logical_screen.y_dots);
}

void free_pixel_grid()
{
    s_grid_x0.clear();
    s_grid_y0.clear();
    s_grid_x1.clear();
    s_grid_y1.clear();
}

void fill_pixel_grid()
{
    s_grid_x0[0] = g_image_region.m_min.x; // fill up the x, y grids
    s_grid_y0[0] = g_image_region.m_max.y;
    s_grid_y1[0] = 0;
    s_grid_x1[0] = 0;
    for (int i = 1; i < g_logical_screen.x_dots; i++)
    {
        s_grid_x0[i] = s_grid_x0[0] + i * static_cast<double>(g_delta_x);
        s_grid_y1[i] = s_grid_y1[0] - i * static_cast<double>(g_delta_y2);
    }
    for (int i = 1; i < g_logical_screen.y_dots; i++)
    {
        s_grid_y0[i] = s_grid_y0[0] - i * static_cast<double>(g_delta_y);
        s_grid_x1[i] = s_grid_x1[0] + i * static_cast<double>(g_delta_x2);
    }
}

} // namespace id::engine
