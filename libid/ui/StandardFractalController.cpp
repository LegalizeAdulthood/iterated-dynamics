// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/StandardFractalController.h"

#include "engine/fractalb.h"
#include "engine/SoundOutput.h"
#include "engine/StandardFractal.h"
#include "misc/Driver.h"
#include "ui/calculate_image.h"
#include "ui/diskvid.h"
#include "ui/goodbye.h"
#include "ui/precision_diagnostics.h"
#include "ui/SoundSession.h"
#include "ui/standard_orbit_plot.h"

#include <memory>
#include <stdexcept>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{
namespace
{

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    explicit OrbitToggleKeyboardHandler(StandardFractal &calculation);
    bool handle_key(int key) override;

private:
    StandardFractal &m_calculation;
};

OrbitToggleKeyboardHandler::OrbitToggleKeyboardHandler(StandardFractal &calculation) :
    m_calculation{calculation}
{
}

bool OrbitToggleKeyboardHandler::handle_key(int key)
{
    if (g_show_orbit)
    {
        g_sound->mute();
        m_calculation.scrub_overlay();
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

StandardFractalController::StandardFractalController(StandardFractal &calculation, SoundSession &sound) :
    m_calculation{calculation},
    m_sound{sound}
{
}

void StandardFractalController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The standard fractal controller has already started");
    }
    reset_calc_interrupted();
    reset_orbit_delay();
    m_main_loop_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
    m_orbit_toggle_scope.emplace(std::make_shared<OrbitToggleKeyboardHandler>(m_calculation));
    m_phase = Phase::ACTIVE;
    try
    {
        complete_image_setup(m_calculation, m_calculation.resume());
        if (show_precision_diagnostics())
        {
            goodbye();
        }
        if (m_calculation.done())
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

void StandardFractalController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The standard fractal controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        if (!m_sound.wait_until_ready())
        {
            m_calculation.scrub_overlay();
            m_calculation.suspend();
            close(-1);
            return;
        }
        bool check_interrupted{true};
        if (m_marker.enabled())
        {
            m_marker.iterate(m_calculation);
        }
        else
        {
            m_calculation.iterate();
        }
        if (m_calculation.potential_storage_required())
        {
            m_calculation.complete_potential_storage(prepare_potential_storage());
            return;
        }
        display_disk_storage_progress();
        if (!g_precision_diagnostics.empty() && show_precision_diagnostics())
        {
            goodbye();
        }
        if (m_calculation.consume_orbit_overlay_displayed())
        {
            pace_orbit_display();
            check_interrupted = false;
        }
        if (!m_calculation.done())
        {
            const bool interrupted{check_interrupted ? calc_interrupted() : calc_interrupt_requested()};
            if (interrupted)
            {
                suspend();
                return;
            }
        }
        else
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

bool StandardFractalController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int StandardFractalController::result() const
{
    if (!done())
    {
        throw std::logic_error("The standard fractal controller has not completed");
    }
    return m_result;
}

void StandardFractalController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active standard fractal controller");
    }
    m_sound.cancel();
    m_marker.restore();
    m_calculation.scrub_overlay();
    m_calculation.suspend();
    close(-1);
}

void StandardFractalController::finish()
{
    const bool completed{m_sound.finish()};
    m_calculation.finish();
    close(completed ? 0 : -1);
}

void StandardFractalController::abort()
{
    m_marker.restore();
    m_sound.cancel();
    m_calculation.scrub_overlay();
    m_calculation.abort();
    close(-1);
}

void StandardFractalController::close(const int result)
{
    m_marker.restore();
    m_orbit_toggle_scope.reset();
    m_main_loop_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
