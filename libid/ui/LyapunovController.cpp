// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/LyapunovController.h"

#include "engine/calcfrac.h"
#include "engine/StandardFractal.h"
#include "fractals/lyapunov.h"
#include "misc/Driver.h"
#include "ui/calculate_image.h"
#include "ui/diskvid.h"
#include "ui/standard_orbit_plot.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::ui
{
namespace
{

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(int key) override;
};

bool OrbitToggleKeyboardHandler::handle_key(int key)
{
    if (g_show_orbit)
    {
        scrub_orbit_display();
    }
    if (key != 'o' && key != 'O')
    {
        return false;
    }
    if (!driver_is_disk())
    {
        g_show_orbit = !g_show_orbit;
    }
    return true;
}

} // namespace

LyapunovController::LyapunovController(Lyapunov &lyapunov) :
    m_lyapunov{lyapunov}
{
}

void LyapunovController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Lyapunov controller has already started");
    }
    reset_calc_interrupted();
    m_main_loop_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
    m_orbit_toggle_scope.emplace(std::make_shared<OrbitToggleKeyboardHandler>());
    m_phase = Phase::ACTIVE;
    try
    {
        complete_image_setup(m_lyapunov.calculation(), m_lyapunov.resume());
        if (m_lyapunov.done())
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

void LyapunovController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Lyapunov controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        if (m_marker.enabled())
        {
            m_marker.iterate(m_lyapunov.calculation());
        }
        else
        {
            m_lyapunov.iterate();
        }
        display_disk_storage_progress();
        if (!m_lyapunov.done() && calc_interrupted())
        {
            suspend();
            return;
        }
        if (m_lyapunov.done())
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

bool LyapunovController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int LyapunovController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Lyapunov controller has not completed");
    }
    return m_result;
}

void LyapunovController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Lyapunov controller");
    }
    m_marker.restore();
    m_lyapunov.suspend();
    close(-1);
}

void LyapunovController::finish()
{
    m_lyapunov.finish();
    close(0);
}

void LyapunovController::abort()
{
    m_marker.restore();
    scrub_orbit_display();
    m_lyapunov.calculation().abort();
    close(-1);
}

void LyapunovController::close(const int result)
{
    m_marker.restore();
    m_orbit_toggle_scope.reset();
    m_main_loop_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
