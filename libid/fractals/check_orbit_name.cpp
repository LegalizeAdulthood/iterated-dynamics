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

static int build_fractal_list(FractalType fractals[], const char *name_ptr[])
{
    int num_fractals = 0;
    for (int i = 0; i < g_num_fractal_types; i++)
    {
        const FractalSpecific &fs{g_fractal_specific[i]};
        if (bit_set(fs.flags, FractalFlags::OK_JB))
        {
            fractals[num_fractals] = fs.type;
            name_ptr[num_fractals] = fs.name;
            num_fractals++;
            if (num_fractals >= MAX_FRACTALS)
            {
                break;
            }
        }
    }
    return num_fractals;
}

bool check_orbit_name(const char *orbit_name)
{
    const char *names[MAX_FRACTALS];
    FractalType types[MAX_FRACTALS];

    int num_types = build_fractal_list(types, names);
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
