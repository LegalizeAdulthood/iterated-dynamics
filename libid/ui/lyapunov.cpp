// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/lyapunov.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "fractals/lyapunov.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"

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
    bool handle_key(int key) override
    {
        if (g_show_orbit)
        {
            scrub_orbit();
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
};

} // namespace

int lyapunov_fractal()
{
    reset_calc_interrupted();
    auto main_loop_handler{std::make_shared<MainLoopKeyboardHandler>()};
    auto orbit_toggle_handler{std::make_shared<OrbitToggleKeyboardHandler>()};
    ScopedKeyboardHandler main_loop_scope{main_loop_handler};
    ScopedKeyboardHandler orbit_toggle_scope{orbit_toggle_handler};

    Lyapunov lyapunov;
    lyapunov.resume();

    while (!lyapunov.done())
    {
        lyapunov.iterate();
        if (!lyapunov.done() && calc_interrupted())
        {
            lyapunov.suspend();
            return -1;
        }
    }
    return 0;
}

} // namespace id::ui
