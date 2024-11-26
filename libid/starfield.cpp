// SPDX-License-Identifier: GPL-3.0-only
//
#include "starfield.h"

#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "choice_builder.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "helpdefs.h"
#include "id_data.h"
#include "loadmap.h"
#include "slideshw.h"
#include "spindac.h"
#include "stop_msg.h"
#include "value_saver.h"
#include "video.h"

#include <cstdlib>

const std::string_view g_altern_map_file{"altern.map"};

static int s_distribution{30};
static int s_slope{25};
static long s_concentration{};
static int s_offset{};
static double s_starfield_values[4]{30.0, 100.0, 5.0, 0.0};

//
// Generate a gaussian distributed number.
// The right half of the distribution is folded onto the lower half.
// That is, the curve slopes up to the peak and then drops to 0.
// The larger slope is, the smaller the standard deviation.
// The values vary from 0+offset to range+offset, with the peak
// at range+offset.
// To make this more complicated, you only have a
// 1 in distribution*(1-probability/range*concentration)+1 chance of getting a
// Gaussian; otherwise you just get offset.
//
static int gausian_number(int probability, int range)
{
    long p;

    p = divide((long)probability << 16, (long)range << 16, 16);
    p = multiply(p, s_concentration, 16);
    p = multiply((long)s_distribution << 16, p, 16);
    if (!(rand15() % (s_distribution - (int)(p >> 16) + 1)))
    {
        long Accum = 0;
        for (int n = 0; n < s_slope; n++)
        {
            Accum += rand15();
        }
        Accum /= s_slope;
        int r = (int)(multiply((long)range << 15, Accum, 15) >> 14);
        r = r - range;
        if (r < 0)
        {
            r = -r;
        }
        return range - r + s_offset;
    }
    return s_offset;
}

int starfield()
{   
    int c;
    g_busy = true;
    if (s_starfield_values[0] <   1.0)
    {
        s_starfield_values[0] =   1.0;
    }
    if (s_starfield_values[0] > 100.0)
    {
        s_starfield_values[0] = 100.0;
    }
    if (s_starfield_values[1] <   1.0)
    {
        s_starfield_values[1] =   1.0;
    }
    if (s_starfield_values[1] > 100.0)
    {
        s_starfield_values[1] = 100.0;
    }
    if (s_starfield_values[2] <   1.0)
    {
        s_starfield_values[2] =   1.0;
    }
    if (s_starfield_values[2] > 100.0)
    {
        s_starfield_values[2] = 100.0;
    }

    s_distribution = (int)(s_starfield_values[0]);
    s_concentration  = (long)(((s_starfield_values[1]) / 100.0) * (1L << 16));
    s_slope = (int)(s_starfield_values[2]);

    if (validate_luts(g_altern_map_file.data()))
    {
        stopmsg("Unable to load ALTERN.MAP");
        g_busy = false;
        return -1;
    }
    spin_dac(0, 1);                 // load it, but don't spin
    for (g_row = 0; g_row < g_logical_screen_y_dots; g_row++)
    {
        for (g_col = 0; g_col < g_logical_screen_x_dots; g_col++)
        {
            if (driver_key_pressed())
            {
                driver_buzzer(buzzer_codes::INTERRUPT);
                g_busy = false;
                return 1;
            }
            c = getcolor(g_col, g_row);
            if (c == g_inside_color)
            {
                c = g_colors-1;
            }
            g_put_color(g_col, g_row, gausian_number(c, g_colors));
        }
    }
    driver_buzzer(buzzer_codes::COMPLETE);
    g_busy = false;
    return 0;
}

int get_starfield_params()
{
    if (g_colors < 255)
    {
        stopmsg("starfield requires 256 color mode");
        return -1;
    }

    ChoiceBuilder<3> builder;
    builder.float_number("Star Density in Pixels per Star", s_starfield_values[0])
        .float_number("Percent Clumpiness", s_starfield_values[1])
        .float_number("Ratio of Dim stars to Bright", s_starfield_values[2]);
    driver_stack_screen();
    ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_STARFIELD};
    int const choice = builder.prompt("Starfield Parameters");
    driver_unstack_screen();
    if (choice < 0)
    {
        return -1;
    }
    for (int i = 0; i < 3; i++)
    {
        s_starfield_values[i] = builder.read_float_number();
    }

    return 0;
}
