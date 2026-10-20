// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/standard_fractal.h"

#include "engine/calcfrac.h"
#include "engine/OrbitPlot.h"
#include "engine/SoundOutput.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/ShowDotController.h"
#include "ui/SoundSession.h"
#include "ui/standard_orbit_plot.h"

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
    explicit OrbitToggleKeyboardHandler(StandardFractal &standard_fractal) :
        m_standard_fractal{standard_fractal}
    {
    }

    bool handle_key(int key) override
    {
        if (g_show_orbit)
        {
            g_sound->mute();
            m_standard_fractal.scrub_overlay();
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
    StandardFractal &m_standard_fractal;
};

} // namespace

int standard_fractal()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return standard_fractal(sound);
}

int standard_fractal(SoundSession &sound)
{
    reset_calc_interrupted();
    reset_orbit_delay();
    StandardFractal standard_fractal;
    ShowDotController marker;
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    KeyboardHandlerPtr orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>(standard_fractal)};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    standard_fractal.resume();

    try
    {
        while (!standard_fractal.done())
        {
            if (!sound.wait_until_ready())
            {
                standard_fractal.scrub_overlay();
                standard_fractal.suspend();
                return -1;
            }
            bool check_interrupted = true;
            if (marker.enabled())
            {
                marker.iterate(standard_fractal);
            }
            else
            {
                standard_fractal.iterate();
            }
            if (standard_fractal.consume_orbit_overlay_displayed())
            {
                pace_orbit_display();
                check_interrupted = false;
            }
            if (!standard_fractal.done())
            {
                const bool interrupted{check_interrupted ? calc_interrupted() : calc_interrupt_requested()};
                if (interrupted)
                {
                    sound.cancel();
                    standard_fractal.scrub_overlay();
                    standard_fractal.suspend();
                    return -1;
                }
            }
        }
    }
    catch (...)
    {
        marker.restore();
        sound.cancel();
        standard_fractal.scrub_overlay();
        standard_fractal.abort();
        throw;
    }
    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
