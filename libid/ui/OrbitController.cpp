// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/OrbitController.h"

#include "engine/NonStandardFractal.h"
#include "ui/diskvid.h"
#include "ui/SoundSession.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;

namespace id::ui
{

OrbitController::OrbitController(NonStandardFractal &calculation, SoundSession &sound) :
    m_calculation{calculation},
    m_sound{sound}
{
}

void OrbitController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The orbit controller has already started");
    }
    reset_calc_interrupted();
    m_keyboard_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
    m_phase = Phase::ACTIVE;
    try
    {
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

void OrbitController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The orbit controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
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

bool OrbitController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int OrbitController::result() const
{
    if (!done())
    {
        throw std::logic_error("The orbit controller has not completed");
    }
    return m_result;
}

void OrbitController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active orbit controller");
    }
    m_sound.cancel();
    m_calculation.suspend();
    close(-1);
}

void OrbitController::finish()
{
    close(m_sound.finish() ? 0 : -1);
}

void OrbitController::abort()
{
    close(-1);
    m_calculation.finish(true);
    m_sound.cancel();
}

void OrbitController::close(const int result)
{
    m_keyboard_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
