// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/CellularController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/stop_msg.h"
#include "ui/thinking.h"

#include <new>
#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

CellularController::CellularController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

CellularController::~CellularController()
{
    clear_thinking();
}

void CellularController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Cellular controller has already started");
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(1);
    }
}

void CellularController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Cellular controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        update_thinking();
        display_disk_storage_progress();
        m_calculation.iterate();
        if (m_calculation.done())
        {
            close(1);
            return;
        }
        if (driver_key_pressed())
        {
            m_calculation.suspend();
            close(-1);
        }
    }
    catch (const std::bad_alloc &)
    {
        fail("Insufficient free memory for calculation");
    }
    catch (const std::exception &error)
    {
        fail(error.what());
    }
    catch (...)
    {
        fail("Unknown error in cellular calculation");
    }
}

bool CellularController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int CellularController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Cellular controller has not completed");
    }
    return m_result;
}

void CellularController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Cellular controller");
    }
    try
    {
        m_calculation.suspend();
        close(-1);
    }
    catch (const std::bad_alloc &)
    {
        fail("Insufficient free memory for calculation");
    }
    catch (const std::exception &error)
    {
        fail(error.what());
    }
    catch (...)
    {
        fail("Unknown error in cellular calculation");
    }
}

void CellularController::update_thinking()
{
    if (m_calculation.preparing())
    {
        m_thinking_active = true;
        thinking("Cellular thinking (higher start row takes longer)");
    }
    else
    {
        clear_thinking();
    }
}

void CellularController::clear_thinking()
{
    if (m_thinking_active)
    {
        m_thinking_active = false;
        thinking_end();
    }
}

void CellularController::fail(const std::string &message)
{
    close(-1);
    g_calc_status = CalcStatus::NON_RESUMABLE;
    m_calculation.finish(true);
    stop_msg(message);
}

void CellularController::close(const int result)
{
    m_result = result;
    m_phase = Phase::COMPLETE;
    clear_thinking();
}

} // namespace id::ui
