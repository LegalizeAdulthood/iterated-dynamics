// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/frothy_basin.h"

#include "engine/NonStandardFractal.h"
#include "engine/SoundOutput.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"
#include "ui/standard_orbit_plot.h"
#include "ui/stop_msg.h"

#include <memory>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

namespace
{

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    explicit OrbitToggleKeyboardHandler(NonStandardFractal &calculation) :
        m_calculation{calculation}
    {
    }

    bool handle_key(int key) override
    {
        if (g_show_orbit)
        {
            g_sound->mute();
            m_calculation.scrub_overlay();
        }
        if (key != 'o' && key != 'O')
        {
            return false;
        }
        if (!driver_is_disk())
        {
            g_show_orbit = !g_show_orbit;
        }
        return true;
    }

private:
    NonStandardFractal &m_calculation;
};

} // namespace

int froth_type(NonStandardFractal &calculation)
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return froth_type(calculation, sound);
}

int froth_type(NonStandardFractal &calculation, SoundSession &sound)
{
    reset_calc_interrupted();
    reset_orbit_delay();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    KeyboardHandlerPtr orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>(calculation)};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    if (const std::string diagnostic{calculation.prepare_palette()}; !diagnostic.empty())
    {
        stop_msg(diagnostic);
    }
    while (!calculation.done())
    {
        if (!sound.wait_until_ready())
        {
            calculation.scrub_overlay();
            calculation.suspend();
            return -1;
        }
        bool check_interrupted = true;
        calculation.iterate();
        display_disk_storage_progress();
        if (calculation.consume_orbit_overlay_displayed())
        {
            pace_orbit_display();
            check_interrupted = false;
        }
        if (!calculation.done())
        {
            const bool interrupted{check_interrupted ? calc_interrupted() : calc_interrupt_requested()};
            if (interrupted)
            {
                sound.cancel();
                calculation.scrub_overlay();
                calculation.suspend();
                return -1;
            }
        }
    }

    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
