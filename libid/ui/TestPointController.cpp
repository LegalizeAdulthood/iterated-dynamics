// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/TestPointController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

TestPointController::TestPointController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void TestPointController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The test-point controller has already started");
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void TestPointController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The test-point controller has not started");
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
        m_calculation.finish(true);
        close(-1);
        throw;
    }
}

bool TestPointController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int TestPointController::result() const
{
    if (!done())
    {
        throw std::logic_error("The test-point controller has not completed");
    }
    return m_result;
}

void TestPointController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active test-point controller");
    }
    m_calculation.suspend();
    close(-1);
}

void TestPointController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
