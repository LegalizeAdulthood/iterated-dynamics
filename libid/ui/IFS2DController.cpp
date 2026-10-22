// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/IFS2DController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

IFS2DController::IFS2DController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void IFS2DController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The IFS2D controller has already started");
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void IFS2DController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The IFS2D controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        if (driver_key_pressed())
        {
            suspend();
            return;
        }
        m_calculation.iterate();
        display_disk_storage_progress();
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

bool IFS2DController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int IFS2DController::result() const
{
    if (!done())
    {
        throw std::logic_error("The IFS2D controller has not completed");
    }
    return m_result;
}

void IFS2DController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active IFS2D controller");
    }
    m_calculation.suspend();
    close(-1);
}

void IFS2DController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
