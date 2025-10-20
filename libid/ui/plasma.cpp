// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/plasma.h"

#include "fractals/Plasma.h"

#include "engine/VideoInfo.h"
#include "misc/Driver.h"
#include "ui/stop_msg.h"

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

int plasma_type()
{
    if (g_colors < 4)
    {
        stop_msg("Plasma Clouds requires 4 or more color video");
        return -1;
    }

    Plasma plasma;
    while (!plasma.done())
    {
        if (driver_key_pressed())
        {
            return 1;
        }

        plasma.iterate();
    }

    return 0;
}

} // namespace id::ui
