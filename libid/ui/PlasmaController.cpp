// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/PlasmaController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

PlasmaController::PlasmaController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void PlasmaController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Plasma controller has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        if (m_calculation.potential_storage_required())
        {
            m_calculation.complete_potential_storage(prepare_potential_storage());
        }
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

void PlasmaController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Plasma controller has not started");
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

bool PlasmaController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int PlasmaController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Plasma controller has not completed");
    }
    return m_result;
}

void PlasmaController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Plasma controller");
    }
    m_calculation.suspend();
    close(1);
}

void PlasmaController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
