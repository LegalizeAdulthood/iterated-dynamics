// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/check_orbit_name.h"

#include "fractals/fractalp.h"
#include "fractals/jb.h"

#include <cstring>

enum
{
    MAX_FRACTALS = 25
};

static int build_fractal_list(FractalType fractals[], int *last_val, char const *name_ptr[])
{
    int num_fractals = 0;
    for (int i = 0; i < g_num_fractal_types; i++)
    {
        if (bit_set(g_fractal_specific[i].flags, FractalFlags::OK_JB))
        {
            const FractalType type{g_fractal_specific[i].type};
            fractals[num_fractals] = type;
            if (type == g_new_orbit_type)
            {
                *last_val = num_fractals;
            }
            name_ptr[num_fractals] = g_fractal_specific[i].name;
            num_fractals++;
            if (num_fractals >= MAX_FRACTALS)
            {
                break;
            }
        }
    }
    return num_fractals;
}

bool check_orbit_name(char const *orbit_name)
{
    char const *names[MAX_FRACTALS];
    FractalType types[MAX_FRACTALS];
    int last_val;

    int num_types = build_fractal_list(types, &last_val, names);
    bool bad = true;
    for (int i = 0; i < num_types; i++)
    {
        if (std::strcmp(orbit_name, names[i]) == 0)
        {
            g_new_orbit_type = types[i];
            bad = false;
            break;
        }
    }
    return bad;
}
