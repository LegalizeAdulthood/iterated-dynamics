// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/StereoController.h"

#include "engine/NonStandardFractal.h"
#include "geometry/plot3d.h"
#include "ui/diskvid.h"
#include "ui/KeyboardHandler.h"
#include "ui/SoundSession.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;
using namespace id::geometry;

namespace id::ui
{

StereoController::StereoController(NonStandardFractal &calculation, SoundSession &sound) :
    m_calculation(calculation),
    m_sound(sound)
{
}

int StereoController::run()
{
    if (m_calculation.done())
    {
        throw std::logic_error("Stereo rendering requires an active calculation");
    }
    reset_calc_interrupted();
    const KeyboardHandlerPtr handler{std::make_shared<MainLoopKeyboardHandler>()};
    const ScopedKeyboardHandler scope{handler};
    try
    {
        setup_stereo_palette();
        while (!m_calculation.done())
        {
            if (m_calculation.view_done())
            {
                if (!m_sound.wait_until_ready())
                {
                    m_calculation.suspend();
                    return -1;
                }
                if (g_glasses_type == GlassesType::PHOTO)
                {
                    lorenz_stereo_photographer_prompt();
                }
                m_calculation.next_view();
                setup_stereo_palette();
            }
            if (!m_sound.wait_until_ready() || calc_interrupted())
            {
                m_sound.cancel();
                m_calculation.suspend();
                return -1;
            }
            m_calculation.iterate();
            display_disk_storage_progress();
        }
        if (!m_sound.finish())
        {
            m_calculation.suspend();
            return -1;
        }
        if (g_glasses_type == GlassesType::PHOTO)
        {
            stop_msg(StopMsgFlags::INFO_ONLY, "Second image (right eye) is ready");
        }
        return 0;
    }
    catch (...)
    {
        m_calculation.finish(true);
        m_sound.cancel();
        throw;
    }
}

} // namespace id::ui
