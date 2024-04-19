#include "find_extra_param.h"

#include "port.h"
#include "prototyp.h"

#include "fractalp.h"
#include "id.h"

int find_extra_param(fractal_type type)
{
    int ret = -1;
    if (bit_set(g_fractal_specific[+type].flags, fractal_flags::MORE))
    {
        fractal_type curtyp;
        int i = -1;
        while ((curtyp = g_more_fractal_params[++i].type) != type && curtyp != fractal_type::NOFRACTAL);
        if (curtyp == type)
        {
            ret = i;
        }
    }
    return ret;
}
