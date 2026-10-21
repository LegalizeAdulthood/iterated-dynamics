// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/non_standard_fractal.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/bifurcation.h"
#include "ui/calculate_image.h"
#include "ui/popcorn.h"
#include "ui/standard_orbit_plot.h"

#include <memory>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

namespace
{

class NonStandardRenderKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(int key) override
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

int non_standard_fractal()
{
    auto keyboard_handler{std::make_shared<NonStandardRenderKeyboardHandler>()};
    ScopedKeyboardHandler keyboard_scope{keyboard_handler};

    NonStandardFractal non_standard_fractal;
    try
    {
        complete_image_setup(non_standard_fractal, non_standard_fractal.start());
        switch (non_standard_fractal.select())
        {
        case NonStandardFractal::Operation::BIFURCATION:
            bifurcation_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::POPCORN:
            popcorn_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::LEGACY:
            non_standard_fractal.render_legacy();
            break;
        }

        if (driver_key_pressed() != 0)
        {
            dispatch_keyboard_key(driver_get_key());
        }
        non_standard_fractal.finish(keyboard_handler->interrupted());
    }
    catch (...)
    {
        g_calc_status = CalcStatus::NON_RESUMABLE;
        non_standard_fractal.finish(true);
        throw;
    }

    return non_standard_fractal.completed() ? 0 : -1;
}

} // namespace id::ui
