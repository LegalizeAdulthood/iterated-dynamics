// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/check_orbit_name.h"

#include "fractals/fractalp.h"
#include "fractals/julibrot.h"

namespace id::fractals
{

bool check_orbit_name(const std::string_view orbit_name)
{
    for (int i = 0; i < g_num_fractal_types; i++)
    {
        if (const FractalSpecific &fs{g_fractal_specific[i]};
            bit_set(fs.flags, FractalFlags::OK_JB) && orbit_name == fs.name)
        {
            g_new_orbit_type = fs.type;
            return false; // found a match
        }
    }
    return true;
}

} // namespace id::fractals
