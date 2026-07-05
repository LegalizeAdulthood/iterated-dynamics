// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/standard_fractal.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/standard_orbit_plot.h"

#include <chrono>
#include <memory>
#include <thread>

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

void wait_for_show_dot_delay()
{
    driver_flush();
    driver_delay(0);
    std::this_thread::sleep_for(std::chrono::microseconds(g_orbit_delay * 100));
    driver_delay(0);
}

} // namespace

int standard_fractal()
{
    reset_calc_interrupted();
    reset_orbit_delay();
    StandardFractal standard_fractal;
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    auto orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>(standard_fractal)};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    standard_fractal.resume();

    while (!standard_fractal.done())
    {
        bool check_interrupted = true;
        if (standard_fractal.orbit_display_pacing_pending())
        {
            pace_orbit_display();
            standard_fractal.complete_orbit_display_pacing();
            check_interrupted = false;
        }
        else if (standard_fractal.show_dot_pacing_pending())
        {
            wait_for_show_dot_delay();
            standard_fractal.complete_show_dot_pacing();
            continue;
        }
        else
        {
            standard_fractal.iterate();
        }
        if (!standard_fractal.done() && !standard_fractal.orbit_display_pacing_pending())
        {
            const bool interrupted{check_interrupted ? calc_interrupted() : calc_interrupt_requested()};
            if (interrupted)
            {
                standard_fractal.suspend();
                return -1;
            }
        }
    }
    return 0;
}

} // namespace id::ui
