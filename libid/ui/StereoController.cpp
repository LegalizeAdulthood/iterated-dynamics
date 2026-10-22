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

void StereoController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The stereo controller has already started");
    }
    if (m_calculation.done())
    {
        throw std::logic_error("Stereo rendering requires an active calculation");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        reset_calc_interrupted();
        m_keyboard_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
        setup_stereo_palette();
    }
    catch (...)
    {
        abort();
        throw;
    }
}

void StereoController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The stereo controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        if (m_calculation.view_done())
        {
            if (!m_sound.wait_until_ready())
            {
                suspend();
                return;
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
            suspend();
            return;
        }
        m_calculation.iterate();
        display_disk_storage_progress();
        if (m_calculation.done())
        {
            finish();
        }
    }
    catch (...)
    {
        abort();
        throw;
    }
}

bool StereoController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int StereoController::result() const
{
    if (!done())
    {
        throw std::logic_error("The stereo controller has not completed");
    }
    return m_result;
}

void StereoController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active stereo controller");
    }
    try
    {
        m_sound.cancel();
        m_calculation.suspend();
        close(-1);
    }
    catch (...)
    {
        abort();
        throw;
    }
}

void StereoController::finish()
{
    if (!m_sound.finish())
    {
        m_calculation.suspend();
        close(-1);
        return;
    }
    if (g_glasses_type == GlassesType::PHOTO)
    {
        stop_msg(StopMsgFlags::INFO_ONLY, "Second image (right eye) is ready");
    }
    close(0);
}

void StereoController::abort()
{
    if (done())
    {
        return;
    }
    close(-1);
    m_calculation.finish(true);
    m_sound.cancel();
}

void StereoController::close(const int result)
{
    m_keyboard_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
