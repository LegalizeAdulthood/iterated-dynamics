// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/diffusion.h"

#include "engine/calcfrac.h"
#include "fractals/Diffusion.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/standard_orbit_plot.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{

namespace
{

class DiffusionKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(const int key) override
    {
        if (g_show_orbit)
        {
            scrub_orbit_display();
        }
        if (key == 'o' || key == 'O')
        {
            if (!driver_is_disk())
            {
                g_show_orbit = !g_show_orbit;
            }
            return true;
        }

        driver_unget_key(key);
        set_calc_interrupted();
        m_interrupted = true;
        return true;
    }

    bool interrupted() const
    {
        return m_interrupted;
    }

private:
    bool m_interrupted{};
};

} // namespace

int diffusion_type()
{
    reset_calc_interrupted();
    Diffusion d;
    auto keyboard_handler{std::make_shared<DiffusionKeyboardHandler>()};
    ScopedKeyboardHandler keyboard_scope{keyboard_handler};

    while (d.iterate())
    {
        dispatch_pending_keyboard_input();
        if (keyboard_handler->interrupted())
        {
            d.suspend();
            return 1;
        }
    }

    return 0;
}

} // namespace id::ui
