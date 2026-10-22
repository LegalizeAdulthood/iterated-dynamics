// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/non_standard_fractal.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/KeyboardHandler.h"
#include "ui/ant.h"
#include "ui/bifurcation.h"
#include "ui/calculate_image.h"
#include "ui/cellular.h"
#include "ui/diffusion.h"
#include "ui/dynamic2d.h"
#include "ui/frothy_basin.h"
#include "ui/ifs2d.h"
#include "ui/inverse_julia.h"
#include "ui/lsystem.h"
#include "ui/plasma.h"
#include "ui/orbit2d.h"
#include "ui/popcorn.h"
#include "ui/standard_orbit_plot.h"
#include "ui/standard_4d.h"
#include "ui/SoundSession.h"
#include "ui/StereoController.h"
#include "ui/testpt.h"

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

        case NonStandardFractal::Operation::DIFFUSION:
            diffusion_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::TEST_POINT:
            test_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::ANT:
            ant_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::CELLULAR:
            cellular_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::PLASMA:
            plasma_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::L_SYSTEM:
            lsystem_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::FROTHY_BASIN:
            froth_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::IFS_2D:
            ifs2d(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::IFS_3D:
        case NonStandardFractal::Operation::ORBIT_3D:
        {
            SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
            StereoController controller{non_standard_fractal, sound};
            controller.run();
            break;
        }

        case NonStandardFractal::Operation::ORBIT_2D:
        {
            SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
            orbit2d_type(non_standard_fractal, sound);
            break;
        }

        case NonStandardFractal::Operation::DYNAMIC_2D:
        {
            SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
            dynamic2d_type(non_standard_fractal, sound);
            break;
        }

        case NonStandardFractal::Operation::INVERSE_JULIA:
            inverse_julia_fractal_type(non_standard_fractal);
            break;

        case NonStandardFractal::Operation::STANDARD_4D:
            standard_4d_type(non_standard_fractal);
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
