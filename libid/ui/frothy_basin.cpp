// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/frothy_basin.h"

#include "engine/calcfrac.h"
#include "engine/SoundOutput.h"
#include "fractals/FrothyBasin.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"
#include "ui/standard_orbit_plot.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

namespace
{

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    explicit OrbitToggleKeyboardHandler(FrothyBasin &frothy_basin) :
        m_frothy_basin{frothy_basin}
    {
    }

    bool handle_key(int key) override
    {
        if (g_show_orbit)
        {
            g_sound->mute();
            m_frothy_basin.scrub_overlay();
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
    FrothyBasin &m_frothy_basin;
};

} // namespace

int froth_type()
{
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    return froth_type(sound);
}

int froth_type(SoundSession &sound)
{
    reset_calc_interrupted();
    reset_orbit_delay();
    KeyboardHandlerPtr main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    KeyboardHandlerPtr orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>(g_frothy_basin)};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    if (!g_frothy_basin.per_image())
    {
        sound.cancel();
        return -1;
    }

    g_frothy_basin.resume();
    while (!g_frothy_basin.done())
    {
        if (!sound.wait_until_ready())
        {
            g_frothy_basin.scrub_overlay();
            g_frothy_basin.suspend();
            return -1;
        }
        bool check_interrupted = true;
        g_frothy_basin.iterate();
        if (g_frothy_basin.consume_orbit_overlay_displayed())
        {
            pace_orbit_display();
            check_interrupted = false;
        }
        if (!g_frothy_basin.done())
        {
            const bool interrupted{check_interrupted ? calc_interrupted() : calc_interrupt_requested()};
            if (interrupted)
            {
                sound.cancel();
                g_frothy_basin.scrub_overlay();
                g_frothy_basin.suspend();
                return -1;
            }
        }
    }

    return sound.finish() ? 0 : -1;
}

} // namespace id::ui
