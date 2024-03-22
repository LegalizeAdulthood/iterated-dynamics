#include "volterra_lotka.h"

#include "port.h"
#include "prototyp.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "fractals.h"
#include "id_data.h"

// Beauty of Fractals pp. 125 - 127
int VLfpFractal()
{
    double a, b, ab, half, u, w, xy;

    half = g_params[0] / 2.0;
    xy = g_old_z.x * g_old_z.y;
    u = g_old_z.x - xy;
    w = -g_old_z.y + xy;
    a = g_old_z.x + g_params[1] * u;
    b = g_old_z.y + g_params[1] * w;
    ab = a * b;
    g_new_z.x = g_old_z.x + half * (u + (a - ab));
    g_new_z.y = g_old_z.y + half * (w + (-b + ab));
    return g_bailout_float();
}

bool VLSetup()
{
    if (g_params[0] < 0.0)
    {
        g_params[0] = 0.0;
    }
    if (g_params[1] < 0.0)
    {
        g_params[1] = 0.0;
    }
    if (g_params[0] > 1.0)
    {
        g_params[0] = 1.0;
    }
    if (g_params[1] > 1.0)
    {
        g_params[1] = 1.0;
    }
    g_float_param = &g_param_z1;
    return true;
}
