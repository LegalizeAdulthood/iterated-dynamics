#include "ui/inverse_julia.h"

#include "engine/jiim.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "ui/check_key.h"

using namespace id::engine;
using namespace id::fractals;

namespace id::ui
{

int inverse_julia_fractal_type()
{
    int color = 0;

    if (g_resuming)              // can't resume
    {
        return -1;
    }

    while (color >= 0)       // generate points
    {
        if (check_key())
        {
            free_queue();
            return -1;
        }
        color = g_cur_fractal_specific->orbit_calc();
        g_old_z = g_new_z;
    }
    free_queue();
    return 0;
}

} // namespace id::ui
