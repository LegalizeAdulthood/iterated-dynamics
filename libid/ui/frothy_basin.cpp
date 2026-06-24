// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/frothy_basin.h"

#include "engine/calcfrac.h"
#include "fractals/FrothyBasin.h"
#include "ui/check_key.h"

#include <cstdlib>

using namespace id::engine;
using namespace id::fractals;

namespace id::ui
{

namespace
{

bool keyboard_check_due()
{
    g_keyboard_check_interval -= std::abs(g_real_color_iter);
    return g_keyboard_check_interval <= 0;
}

void keyboard_reset()
{
    g_keyboard_check_interval = g_max_keyboard_check_interval;
}

} // namespace

int froth_type()
{
    if (!g_frothy_basin.per_image())
    {
        return -1;
    }

    keyboard_reset();
    g_frothy_basin.resume();
    while (!g_frothy_basin.done())
    {
        if (check_key())
        {
            g_frothy_basin.suspend();
            return -1;
        }
        g_frothy_basin.iterate();
        if (keyboard_check_due())
        {
            if (check_key())
            {
                g_frothy_basin.suspend();
                return -1;
            }
            keyboard_reset();
        }
    }

    return 0;
}

} // namespace id::ui
