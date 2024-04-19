#include "check_orbit_name.h"

#include "fractalp.h"
#include "jb.h"

#include <cstring>

#define MAXFRACTALS 25

static int build_fractal_list(int fractals[], int *last_val, char const *nameptr[])
{
    int numfractals = 0;
    for (int i = 0; i < g_num_fractal_types; i++)
    {
        if ((g_fractal_specific[i].flags & OKJB) && *g_fractal_specific[i].name != '*')
        {
            fractals[numfractals] = i;
            if (i == +g_new_orbit_type || i == +g_fractal_specific[+g_new_orbit_type].tofloat)
            {
                *last_val = numfractals;
            }
            nameptr[numfractals] = g_fractal_specific[i].name;
            numfractals++;
            if (numfractals >= MAXFRACTALS)
            {
                break;
            }
        }
    }
    return numfractals;
}

bool check_orbit_name(char const *orbitname)
{
    int numtypes;
    char const *nameptr[MAXFRACTALS];
    int fractals[MAXFRACTALS];
    int last_val;

    numtypes = build_fractal_list(fractals, &last_val, nameptr);
    bool bad = true;
    for (int i = 0; i < numtypes; i++)
    {
        if (std::strcmp(orbitname, nameptr[i]) == 0)
        {
            g_new_orbit_type = static_cast<fractal_type>(fractals[i]);
            bad = false;
            break;
        }
    }
    return bad;
}
