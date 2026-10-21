// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/standard_4d.h"

#include "engine/spindac.h"
#include "fractals/julibrot.h"
#include "io/loadmap.h"
#include "misc/Driver.h"
#include "ui/get_3d_params.h"
#include "ui/starfield.h"
#include "ui/stop_msg.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

int standard_4d_type()
{
    const bool red_blue{g_julibrot_3d_mode == Julibrot3DMode::RED_BLUE};
    if (red_blue)
    {
        g_save_dac = SaveDAC::NO;
    }
    if (g_save_dac != SaveDAC::YES)
    {
        const PaletteLoadResult palette{validate_luts(red_blue ? GLASSES1_MAP_NAME : ALTERN_MAP_NAME)};
        if (palette.failed)
        {
            throw std::runtime_error{palette.diagnostic};
        }
        if (!palette.diagnostic.empty())
        {
            stop_msg(palette.diagnostic);
        }
        refresh_dac();
        if (g_save_dac == SaveDAC::NEXT_TIME)
        {
            g_save_dac = SaveDAC::YES;
        }
    }
    Standard4D s4d;

    while (s4d.iterate())
    {
        if (driver_key_pressed())
        {
            return -1;
        }
    }

    return 0;
}

} // namespace id::ui
