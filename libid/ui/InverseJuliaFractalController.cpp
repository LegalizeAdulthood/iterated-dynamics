// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/InverseJuliaFractalController.h"

#include "engine/NonStandardFractal.h"
#include "misc/Driver.h"
#include "ui/inverse_julia.h"
#include "ui/standard_orbit_plot.h"
#include "ui/stop_msg.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{
namespace
{

class InverseJuliaOrbitKeyboardHandler : public KeyboardHandler
{
public:
    explicit InverseJuliaOrbitKeyboardHandler(bool &interrupted);
    bool handle_key(int key) override;

private:
    bool &m_interrupted;
};

InverseJuliaOrbitKeyboardHandler::InverseJuliaOrbitKeyboardHandler(bool &interrupted) :
    m_interrupted{interrupted}
{
}

bool InverseJuliaOrbitKeyboardHandler::handle_key(const int key)
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

InverseJuliaFractalController::InverseJuliaFractalController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void InverseJuliaFractalController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The inverse Julia fractal controller has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        if (m_calculation.inverse_julia_queue_required())
        {
            const bool available{prepare_inverse_julia_queue(32 * 1024UL)};
            if (!available)
            {
                stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER,
                    "Not enough memory: switching to random walk.\n");
            }
            m_calculation.complete_inverse_julia_queue(available);
        }

        reset_calc_interrupted();
        m_keyboard_scope.emplace(std::make_shared<InverseJuliaOrbitKeyboardHandler>(m_interrupted));
        if (m_calculation.done())
        {
            close(m_calculation.completed() ? 0 : -1);
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

void InverseJuliaFractalController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The inverse Julia fractal controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        dispatch_pending_keyboard_input();
        if (m_interrupted)
        {
            suspend();
            return;
        }
        m_calculation.iterate();
        if (m_calculation.done())
        {
            close(m_calculation.completed() ? 0 : -1);
        }
    }
    catch (...)
    {
        close(-1);
        m_calculation.finish(true);
        throw;
    }
}

bool InverseJuliaFractalController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int InverseJuliaFractalController::result() const
{
    if (!done())
    {
        throw std::logic_error("The inverse Julia fractal controller has not completed");
    }
    return m_result;
}

void InverseJuliaFractalController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active inverse Julia fractal controller");
    }
    m_calculation.suspend();
    close(-1);
}

void InverseJuliaFractalController::close(const int result)
{
    m_keyboard_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
