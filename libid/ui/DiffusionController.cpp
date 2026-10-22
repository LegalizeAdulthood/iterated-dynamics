// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/DiffusionController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/standard_orbit_plot.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{
namespace
{

class DiffusionKeyboardHandler : public KeyboardHandler
{
public:
    explicit DiffusionKeyboardHandler(bool &interrupted);
    bool handle_key(int key) override;

private:
    bool &m_interrupted;
};

DiffusionKeyboardHandler::DiffusionKeyboardHandler(bool &interrupted) :
    m_interrupted{interrupted}
{
}

bool DiffusionKeyboardHandler::handle_key(const int key)
{
    if (g_show_orbit)
    {
        scrub_orbit_display();
    }
    if (key == 'o' || key == 'O')
    {
        if (!driver_is_disk())
        {
            g_show_orbit = !g_show_orbit;
        }
        return true;
    }

    driver_unget_key(key);
    set_calc_interrupted();
    m_interrupted = true;
    return true;
}

} // namespace

DiffusionController::DiffusionController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void DiffusionController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The diffusion controller has already started");
    }
    reset_calc_interrupted();
    m_keyboard_scope.emplace(std::make_shared<DiffusionKeyboardHandler>(m_interrupted));
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void DiffusionController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The diffusion controller has not started");
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
            dispatch_pending_keyboard_input();
            if (m_interrupted)
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
        close(1);
        throw;
    }
}

bool DiffusionController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int DiffusionController::result() const
{
    if (!done())
    {
        throw std::logic_error("The diffusion controller has not completed");
    }
    return m_result;
}

void DiffusionController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active diffusion controller");
    }
    m_calculation.suspend();
    close(1);
}

void DiffusionController::close(const int result)
{
    m_keyboard_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
