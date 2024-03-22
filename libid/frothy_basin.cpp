// frothy basin routines

#include "frothy_basin.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "fracsubr.h"
#include "fractals.h"
#include "fractint.h"
#include "id_data.h"
#include "loadmap.h"
#include "newton.h"
#include "pixel_grid.h"
#include "spindac.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#define FROTH_BITSHIFT      28
#define FROTH_D_TO_L(x)     ((long)((x)*(1L<<FROTH_BITSHIFT)))
#define FROTH_CLOSE         1e-6      // seems like a good value
#define FROTH_LCLOSE        FROTH_D_TO_L(FROTH_CLOSE)
#define SQRT3               1.732050807568877193
#define FROTH_SLOPE         SQRT3
#define FROTH_LSLOPE        FROTH_D_TO_L(FROTH_SLOPE)
#define FROTH_CRITICAL_A    1.028713768218725  // 1.0287137682187249127
#define froth_top_x_mapping(x)  ((x)*(x)-(x)-3*fsp.fl.f.a*fsp.fl.f.a/4)


struct froth_double_struct
{
    double a;
    double halfa;
    double top_x1;
    double top_x2;
    double top_x3;
    double top_x4;
    double left_x1;
    double left_x2;
    double left_x3;
    double left_x4;
    double right_x1;
    double right_x2;
    double right_x3;
    double right_x4;
};

struct froth_long_struct
{
    long a;
    long halfa;
    long top_x1;
    long top_x2;
    long top_x3;
    long top_x4;
    long left_x1;
    long left_x2;
    long left_x3;
    long left_x4;
    long right_x1;
    long right_x2;
    long right_x3;
    long right_x4;
};

struct froth_struct
{
    int repeat_mapping;
    int altcolor;
    int attractors;
    int shades;
    union
    {
        froth_double_struct f;
        froth_long_struct l;
    } fl;
};

static froth_struct fsp;

// color maps which attempt to replicate the images of James Alexander.
static void set_Froth_palette()
{
    char const *mapname;

    if (g_color_state != 0)   // 0 means g_dac_box matches default
    {
        return;
    }
    if (g_colors >= 16)
    {
        if (g_colors >= 256)
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth6.map";
            }
            else
            {
                mapname = "froth3.map";

            }
        }
        else // colors >= 16
        {
            if (fsp.attractors == 6)
            {
                mapname = "froth616.map";
            }
            else
            {
                mapname = "froth316.map";

            }
        }
        if (ValidateLuts(mapname))
        {
            return;
        }
        g_color_state = 0; // treat map as default
        spindac(0, 1);
    }
}

bool froth_setup()
{
    double sin_theta;
    double cos_theta;

    sin_theta = SQRT3/2; // sin(2*PI/3)
    cos_theta = -0.5;    // cos(2*PI/3)

    // for the all important backwards compatibility
    if (g_params[0] != 2)
    {
        g_params[0] = 1;
    }
    fsp.repeat_mapping = (int)g_params[0] == 2;
    if (g_params[1] != 0)
    {
        g_params[1] = 1;
    }
    fsp.altcolor = (int)g_params[1];
    fsp.fl.f.a = g_params[2];

    fsp.attractors = std::fabs(fsp.fl.f.a) <= FROTH_CRITICAL_A ? (!fsp.repeat_mapping ? 3 : 6)
                         : (!fsp.repeat_mapping ? 2 : 3);

    // new improved values
    // 0.5 is the value that causes the mapping to reach a minimum
    double x0 = 0.5;
    // a/2 is the value that causes the y value to be invariant over the mappings
    fsp.fl.f.halfa = fsp.fl.f.a/2;
    double y0 = fsp.fl.f.halfa;
    fsp.fl.f.top_x1 = froth_top_x_mapping(x0);
    fsp.fl.f.top_x2 = froth_top_x_mapping(fsp.fl.f.top_x1);
    fsp.fl.f.top_x3 = froth_top_x_mapping(fsp.fl.f.top_x2);
    fsp.fl.f.top_x4 = froth_top_x_mapping(fsp.fl.f.top_x3);

    // rotate 120 degrees counter-clock-wise
    fsp.fl.f.left_x1 = fsp.fl.f.top_x1 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x2 = fsp.fl.f.top_x2 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x3 = fsp.fl.f.top_x3 * cos_theta - y0 * sin_theta;
    fsp.fl.f.left_x4 = fsp.fl.f.top_x4 * cos_theta - y0 * sin_theta;

    // rotate 120 degrees clock-wise
    fsp.fl.f.right_x1 = fsp.fl.f.top_x1 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x2 = fsp.fl.f.top_x2 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x3 = fsp.fl.f.top_x3 * cos_theta + y0 * sin_theta;
    fsp.fl.f.right_x4 = fsp.fl.f.top_x4 * cos_theta + y0 * sin_theta;

    // if 2 attractors, use same shades as 3 attractors
    fsp.shades = (g_colors-1) / std::max(3, fsp.attractors);

    // rqlim needs to be at least sq(1+sqrt(1+sq(a))),
    // which is never bigger than 6.93..., so we'll call it 7.0
    if (g_magnitude_limit < 7.0)
    {
        g_magnitude_limit = 7.0;
    }
    set_Froth_palette();
    // make the best of the .map situation
    g_orbit_color = fsp.attractors != 6 && g_colors >= 16 ? (fsp.shades<<1)+1 : g_colors-1;

    if (g_integer_fractal)
    {
        froth_long_struct tmp_l;

        tmp_l.a        = FROTH_D_TO_L(fsp.fl.f.a);
        tmp_l.halfa    = FROTH_D_TO_L(fsp.fl.f.halfa);

        tmp_l.top_x1   = FROTH_D_TO_L(fsp.fl.f.top_x1);
        tmp_l.top_x2   = FROTH_D_TO_L(fsp.fl.f.top_x2);
        tmp_l.top_x3   = FROTH_D_TO_L(fsp.fl.f.top_x3);
        tmp_l.top_x4   = FROTH_D_TO_L(fsp.fl.f.top_x4);

        tmp_l.left_x1  = FROTH_D_TO_L(fsp.fl.f.left_x1);
        tmp_l.left_x2  = FROTH_D_TO_L(fsp.fl.f.left_x2);
        tmp_l.left_x3  = FROTH_D_TO_L(fsp.fl.f.left_x3);
        tmp_l.left_x4  = FROTH_D_TO_L(fsp.fl.f.left_x4);

        tmp_l.right_x1 = FROTH_D_TO_L(fsp.fl.f.right_x1);
        tmp_l.right_x2 = FROTH_D_TO_L(fsp.fl.f.right_x2);
        tmp_l.right_x3 = FROTH_D_TO_L(fsp.fl.f.right_x3);
        tmp_l.right_x4 = FROTH_D_TO_L(fsp.fl.f.right_x4);

        fsp.fl.l = tmp_l;
    }
    return true;
}

void froth_cleanup()
{
}


// Froth Fractal type
int calcfroth()   // per pixel 1/2/g, called with row & col set
{
    int found_attractor = 0;

    if (check_key())
    {
        return -1;
    }

    g_orbit_save_index = 0;
    g_color_iter = 0;
    if (g_show_dot >0)
    {
        (*g_plot)(g_col, g_row, g_show_dot %g_colors);
    }
    if (!g_integer_fractal) // fp mode
    {
        if (g_invert != 0)
        {
            invertz2(&g_tmp_z);
            g_old_z = g_tmp_z;
        }
        else
        {
            g_old_z.x = g_dx_pixel();
            g_old_z.y = g_dy_pixel();
        }

        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);
        while (!found_attractor
            && (g_temp_sqr_x + g_temp_sqr_y < g_magnitude_limit)
            && (g_color_iter < g_max_iterations))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - fsp.fl.f.a*g_old_z.y;
            g_old_z.y += (g_old_z.x+g_old_z.x)*g_old_z.y - fsp.fl.f.a*g_old_z.x;
            g_old_z.x = g_new_z.x;
            if (fsp.repeat_mapping)
            {
                g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - fsp.fl.f.a*g_old_z.y;
                g_old_z.y += (g_old_z.x+g_old_z.x)*g_old_z.y - fsp.fl.f.a*g_old_z.x;
                g_old_z.x = g_new_z.x;
            }

            g_color_iter++;

            if (g_show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                plot_orbit(g_old_z.x, g_old_z.y, -1);
            }

            if (std::fabs(fsp.fl.f.halfa-g_old_z.y) < FROTH_CLOSE
                && g_old_z.x >= fsp.fl.f.top_x1
                && g_old_z.x <= fsp.fl.f.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                    || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (g_old_z.x <= fsp.fl.f.top_x3)
                {
                    found_attractor = 1;
                }
                else if (g_old_z.x >= fsp.fl.f.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (std::fabs(FROTH_SLOPE*g_old_z.x - fsp.fl.f.a - g_old_z.y) < FROTH_CLOSE
                && g_old_z.x <= fsp.fl.f.right_x1
                && g_old_z.x >= fsp.fl.f.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (g_old_z.x >= fsp.fl.f.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (g_old_z.x <= fsp.fl.f.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (std::fabs(-FROTH_SLOPE*g_old_z.x - fsp.fl.f.a - g_old_z.y) < FROTH_CLOSE
                && g_old_z.x <= fsp.fl.f.left_x1
                && g_old_z.x >= fsp.fl.f.left_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (g_old_z.x >= fsp.fl.f.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (g_old_z.x <= fsp.fl.f.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            g_temp_sqr_x = sqr(g_old_z.x);
            g_temp_sqr_y = sqr(g_old_z.y);
        }
    }
    else // integer mode
    {
        if (g_invert != 0)
        {
            invertz2(&g_tmp_z);
            g_l_old_z.x = (long)(g_tmp_z.x * g_fudge_factor);
            g_l_old_z.y = (long)(g_tmp_z.y * g_fudge_factor);
        }
        else
        {
            g_l_old_z.x = g_l_x_pixel();
            g_l_old_z.y = g_l_y_pixel();
        }

        g_l_temp_sqr_x = lsqr(g_l_old_z.x);
        g_l_temp_sqr_y = lsqr(g_l_old_z.y);
        g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
        while (!found_attractor
            && (g_l_magnitude < g_l_magnitude_limit)
            && (g_l_magnitude >= 0)
            && (g_color_iter < g_max_iterations))
        {
            // simple formula: z = z^2 + conj(z*(-1+ai))
            // but it's the attractor that makes this so interesting
            g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
            g_l_old_z.y += (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
            g_l_old_z.x = g_l_new_z.x;
            if (fsp.repeat_mapping)
            {
                g_l_magnitude = (g_l_temp_sqr_x = lsqr(g_l_old_z.x)) + (g_l_temp_sqr_y = lsqr(g_l_old_z.y));
                if ((g_l_magnitude > g_l_magnitude_limit) || (g_l_magnitude < 0))
                {
                    break;
                }
                g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
                g_l_old_z.y += (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
                g_l_old_z.x = g_l_new_z.x;
            }
            g_color_iter++;

            if (g_show_orbit)
            {
                if (driver_key_pressed())
                {
                    break;
                }
                iplot_orbit(g_l_old_z.x, g_l_old_z.y, -1);
            }

            if (labs(fsp.fl.l.halfa-g_l_old_z.y) < FROTH_LCLOSE
                && g_l_old_z.x > fsp.fl.l.top_x1
                && g_l_old_z.x < fsp.fl.l.top_x2)
            {
                if ((!fsp.repeat_mapping && fsp.attractors == 2)
                    || (fsp.repeat_mapping && fsp.attractors == 3))
                {
                    found_attractor = 1;
                }
                else if (g_l_old_z.x <= fsp.fl.l.top_x3)
                {
                    found_attractor = 1;
                }
                else if (g_l_old_z.x >= fsp.fl.l.top_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 1;
                    }
                    else
                    {
                        found_attractor = 2;
                    }
                }
            }
            else if (labs(multiply(FROTH_LSLOPE, g_l_old_z.x, g_bit_shift)-fsp.fl.l.a-g_l_old_z.y) < FROTH_LCLOSE
                && g_l_old_z.x <= fsp.fl.l.right_x1
                && g_l_old_z.x >= fsp.fl.l.right_x2)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 3;
                }
                else if (g_l_old_z.x >= fsp.fl.l.right_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 4;
                    }
                }
                else if (g_l_old_z.x <= fsp.fl.l.right_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 6;
                    }
                }
            }
            else if (labs(multiply(-FROTH_LSLOPE, g_l_old_z.x, g_bit_shift)-fsp.fl.l.a-g_l_old_z.y) < FROTH_LCLOSE)
            {
                if (!fsp.repeat_mapping && fsp.attractors == 2)
                {
                    found_attractor = 2;
                }
                else if (fsp.repeat_mapping && fsp.attractors == 3)
                {
                    found_attractor = 2;
                }
                else if (g_l_old_z.x >= fsp.fl.l.left_x3)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 3;
                    }
                    else
                    {
                        found_attractor = 5;
                    }
                }
                else if (g_l_old_z.x <= fsp.fl.l.left_x4)
                {
                    if (!fsp.repeat_mapping)
                    {
                        found_attractor = 2;
                    }
                    else
                    {
                        found_attractor = 3;
                    }
                }
            }
            g_l_temp_sqr_x = lsqr(g_l_old_z.x);
            g_l_temp_sqr_y = lsqr(g_l_old_z.y);
            g_l_magnitude = g_l_temp_sqr_x + g_l_temp_sqr_y;
        }
    }
    if (g_show_orbit)
    {
        scrub_orbit();
    }

    g_real_color_iter = g_color_iter;
    if ((g_keyboard_check_interval -= std::abs((int)g_real_color_iter)) <= 0)
    {
        if (check_key())
        {
            return -1;
        }
        g_keyboard_check_interval = g_max_keyboard_check_interval;
    }

    // inside - Here's where non-palette based images would be nice.  Instead,
    // we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth
    // color maps in attempt to replicate the images of James Alexander.
    if (found_attractor)
    {
        if (g_colors >= 256)
        {
            if (!fsp.altcolor)
            {
                if (g_color_iter > fsp.shades)
                {
                    g_color_iter = fsp.shades;
                }
            }
            else
            {
                g_color_iter = fsp.shades * g_color_iter / g_max_iterations;
            }
            if (g_color_iter == 0)
            {
                g_color_iter = 1;
            }
            g_color_iter += fsp.shades * (found_attractor-1);
        }
        else if (g_colors >= 16)
        {
            // only alternate coloring scheme available for 16 colors
            long lshade;

            // Trying to make a better 16 color distribution.
            // Since their are only a few possiblities, just handle each case.
            // This is a mostly guess work here.
            lshade = (g_color_iter<<16)/g_max_iterations;
            if (fsp.attractors != 6) // either 2 or 3 attractors
            {
                if (lshade < 2622)         // 0.04
                {
                    g_color_iter = 1;
                }
                else if (lshade < 10486)     // 0.16
                {
                    g_color_iter = 2;
                }
                else if (lshade < 23593)     // 0.36
                {
                    g_color_iter = 3;
                }
                else if (lshade < 41943L)     // 0.64
                {
                    g_color_iter = 4;
                }
                else
                {
                    g_color_iter = 5;
                }
                g_color_iter += 5 * (found_attractor-1);
            }
            else // 6 attractors
            {
                if (lshade < 10486)        // 0.16
                {
                    g_color_iter = 1;
                }
                else
                {
                    g_color_iter = 2;
                }
                g_color_iter += 2 * (found_attractor-1);
            }
        }
        else   // use a color corresponding to the attractor
        {
            g_color_iter = found_attractor;
        }
        g_old_color_iter = g_color_iter;
    }
    else   // outside, or inside but didn't get sucked in by attractor.
    {
        g_color_iter = 0;
    }

    g_color = std::abs((int)(g_color_iter));

    (*g_plot)(g_col, g_row, g_color);

    return g_color;
}

/*
These last two froth functions are for the orbit-in-window feature.
Normally, this feature requires standard_fractal, but since it is the
attractor that makes the frothybasin type so unique, it is worth
putting in as a stand-alone.
*/

int froth_per_pixel()
{
    if (!g_integer_fractal) // fp mode
    {
        g_old_z.x = g_dx_pixel();
        g_old_z.y = g_dy_pixel();
        g_temp_sqr_x = sqr(g_old_z.x);
        g_temp_sqr_y = sqr(g_old_z.y);
    }
    else  // integer mode
    {
        g_l_old_z.x = g_l_x_pixel();
        g_l_old_z.y = g_l_y_pixel();
        g_l_temp_sqr_x = multiply(g_l_old_z.x, g_l_old_z.x, g_bit_shift);
        g_l_temp_sqr_y = multiply(g_l_old_z.y, g_l_old_z.y, g_bit_shift);
    }
    return 0;
}

int froth_per_orbit()
{
    if (!g_integer_fractal) // fp mode
    {
        g_new_z.x = g_temp_sqr_x - g_temp_sqr_y - g_old_z.x - fsp.fl.f.a*g_old_z.y;
        g_new_z.y = 2.0*g_old_z.x*g_old_z.y - fsp.fl.f.a*g_old_z.x + g_old_z.y;
        if (fsp.repeat_mapping)
        {
            g_old_z = g_new_z;
            g_new_z.x = sqr(g_old_z.x) - sqr(g_old_z.y) - g_old_z.x - fsp.fl.f.a*g_old_z.y;
            g_new_z.y = 2.0*g_old_z.x*g_old_z.y - fsp.fl.f.a*g_old_z.x + g_old_z.y;
        }

        g_temp_sqr_x = sqr(g_new_z.x);
        g_temp_sqr_y = sqr(g_new_z.y);
        if (g_temp_sqr_x + g_temp_sqr_y >= g_magnitude_limit)
        {
            return 1;
        }
        g_old_z = g_new_z;
    }
    else  // integer mode
    {
        g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
        g_l_new_z.y = g_l_old_z.y + (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
        if (fsp.repeat_mapping)
        {
            g_l_temp_sqr_x = lsqr(g_l_new_z.x);
            g_l_temp_sqr_y = lsqr(g_l_new_z.y);
            if (g_l_temp_sqr_x + g_l_temp_sqr_y >= g_l_magnitude_limit)
            {
                return 1;
            }
            g_l_old_z = g_l_new_z;
            g_l_new_z.x = g_l_temp_sqr_x - g_l_temp_sqr_y - g_l_old_z.x - multiply(fsp.fl.l.a, g_l_old_z.y, g_bit_shift);
            g_l_new_z.y = g_l_old_z.y + (multiply(g_l_old_z.x, g_l_old_z.y, g_bit_shift)<<1) - multiply(fsp.fl.l.a, g_l_old_z.x, g_bit_shift);
        }
        g_l_temp_sqr_x = lsqr(g_l_new_z.x);
        g_l_temp_sqr_y = lsqr(g_l_new_z.y);
        if (g_l_temp_sqr_x + g_l_temp_sqr_y >= g_l_magnitude_limit)
        {
            return 1;
        }
        g_l_old_z = g_l_new_z;
    }
    return 0;
}
