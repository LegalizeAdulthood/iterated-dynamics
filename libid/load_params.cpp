#include "load_params.h"

#include "port.h"
#include "prototyp.h"

#include "find_extra_param.h"
#include "fractalp.h"
#include "id.h"
#include "id_data.h"
#include "round_float_double.h"

void load_params(fractal_type fractype)
{
    for (int i = 0; i < 4; ++i)
    {
        g_params[i] = g_fractal_specific[+fractype].paramvalue[i];
    }
    int extra = find_extra_param(fractype);
    if (extra > -1)
    {
        for (int i = 0; i < MAX_PARAMS-4; i++)
        {
            g_params[i+4] = g_more_fractal_params[extra].paramvalue[i];
        }
    }
}
