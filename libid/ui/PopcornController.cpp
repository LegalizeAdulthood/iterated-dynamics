// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/PopcornController.h"

#include "engine/NonStandardFractal.h"
#include "ui/diskvid.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;

namespace id::ui
{

PopcornController::PopcornController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void PopcornController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Popcorn controller has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        reset_calc_interrupted();
        m_keyboard_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
        if (m_calculation.done())
        {
            close(0);
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

void PopcornController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Popcorn controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        m_calculation.iterate();
        display_disk_storage_progress();
        if (m_calculation.done())
        {
            close(0);
        }
        else if (calc_interrupted())
        {
            suspend();
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

bool PopcornController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int PopcornController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Popcorn controller has not completed");
    }
    return m_result;
}

void PopcornController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Popcorn controller");
    }
    try
    {
        m_calculation.suspend();
        close(-1);
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

void PopcornController::close(const int result)
{
    m_keyboard_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
