// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/BifurcationController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

BifurcationController::BifurcationController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void BifurcationController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The bifurcation controller has already started");
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void BifurcationController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The bifurcation controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        m_calculation.iterate();
        if (!m_calculation.done())
        {
            display_disk_storage_progress();
            if (driver_key_pressed())
            {
                suspend();
                return;
            }
        }
        if (m_calculation.done())
        {
            close(0);
        }
    }
    catch (...)
    {
        m_calculation.finish(true);
        close(-1);
        throw;
    }
}

bool BifurcationController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int BifurcationController::result() const
{
    if (!done())
    {
        throw std::logic_error("The bifurcation controller has not completed");
    }
    return m_result;
}

void BifurcationController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active bifurcation controller");
    }
    m_calculation.suspend();
    close(-1);
}

void BifurcationController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
