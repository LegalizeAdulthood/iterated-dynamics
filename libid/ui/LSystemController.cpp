// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/LSystemController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/thinking.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

LSystemController::LSystemController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

LSystemController::~LSystemController()
{
    clear_thinking();
}

void LSystemController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The L-system controller has already started");
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void LSystemController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The L-system controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        const bool sizing{m_calculation.sizing()};
        if (update_thinking(sizing))
        {
            suspend();
            return;
        }
        if (!sizing && m_poll_counter++ == 0)
        {
            display_disk_storage_progress();
            if (driver_key_pressed())
            {
                suspend();
                return;
            }
        }
        m_calculation.iterate();
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

bool LSystemController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int LSystemController::result() const
{
    if (!done())
    {
        throw std::logic_error("The L-system controller has not completed");
    }
    return m_result;
}

void LSystemController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active L-system controller");
    }
    try
    {
        m_calculation.suspend();
        close(0);
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

bool LSystemController::update_thinking(const bool sizing)
{
    if (!sizing)
    {
        clear_thinking();
        return false;
    }
    m_thinking_active = true;
    return thinking("L-System thinking (higher orders take longer)");
}

void LSystemController::clear_thinking()
{
    if (m_thinking_active)
    {
        m_thinking_active = false;
        thinking_end();
    }
}

void LSystemController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
    clear_thinking();
}

} // namespace id::ui
