// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ifs3d.h"

#include "engine/NonStandardFractal.h"
#include "geometry/plot3d.h"
#include "ui/diskvid.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"
#include "misc/Driver.h"

using namespace id::engine;
using namespace id::geometry;
using namespace id::misc;

namespace id::ui
{

int ifs3d_calc(NonStandardFractal &calculation)
{
    setup_stereo_palette();
    while (!calculation.done())
    {
        if (calculation.view_done())
        {
            if (g_glasses_type == GlassesType::PHOTO)
            {
                lorenz_stereo_photographer_prompt();
            }
            calculation.next_view();
            setup_stereo_palette();
        }
        // keypress bails out
        if (driver_key_pressed())
        {
            calculation.suspend();
            return -1;
        }
        calculation.iterate();
        display_disk_storage_progress();
    }
    if (g_glasses_type == GlassesType::PHOTO)
    {
        stop_msg(StopMsgFlags::INFO_ONLY, "Second image (right eye) is ready");
    }
    return 0;
}

} // namespace id::ui
