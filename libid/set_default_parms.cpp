#include "set_default_parms.h"

#include "port.h"
#include "prototyp.h"

#include "calc_frac_init.h"
#include "cmdfiles.h"
#include "find_extra_param.h"
#include "fractalp.h"
#include "id.h"
#include "id_data.h"
#include "miscres.h"
#include "round_float_double.h"
#include "zoom.h"

void set_default_parms()
{
    g_x_min = g_cur_fractal_specific->xmin;
    g_x_max = g_cur_fractal_specific->xmax;
    g_y_min = g_cur_fractal_specific->ymin;
    g_y_max = g_cur_fractal_specific->ymax;
    g_x_3rd = g_x_min;
    g_y_3rd = g_y_min;

    if (g_view_crop && g_final_aspect_ratio != g_screen_aspect)
    {
        aspectratio_crop(g_screen_aspect, g_final_aspect_ratio);
    }
    for (int i = 0; i < 4; i++)
    {
        g_params[i] = g_cur_fractal_specific->paramvalue[i];
        if (g_fractal_type != fractal_type::CELLULAR
            && g_fractal_type != fractal_type::FROTH
            && g_fractal_type != fractal_type::FROTHFP
            && g_fractal_type != fractal_type::ANT)
        {
            roundfloatd(&g_params[i]); // don't round cellular, frothybasin or ant
        }
    }
    int extra = find_extra_param(g_fractal_type);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS-4; i++)
        {
            g_params[i+4] = g_more_fractal_params[extra].paramvalue[i];
        }
    }
    if (g_debug_flag != debug_flags::force_arbitrary_precision_math)
    {
        bf_math = bf_math_type::NONE;
    }
    else if (bf_math != bf_math_type::NONE)
    {
        fractal_floattobf();
    }
}
