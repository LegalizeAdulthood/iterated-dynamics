// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/FrothyBasinController.h"

#include "engine/NonStandardFractal.h"
#include "engine/SoundOutput.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/SoundSession.h"
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

class OrbitToggleKeyboardHandler : public KeyboardHandler
{
public:
    explicit OrbitToggleKeyboardHandler(NonStandardFractal &calculation);
    bool handle_key(int key) override;

private:
    NonStandardFractal &m_calculation;
};

OrbitToggleKeyboardHandler::OrbitToggleKeyboardHandler(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

bool OrbitToggleKeyboardHandler::handle_key(const int key)
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

FrothyBasinController::FrothyBasinController(NonStandardFractal &calculation, SoundSession &sound) :
    m_calculation{calculation},
    m_sound{sound}
{
}

void FrothyBasinController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The FrothyBasin controller has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        reset_calc_interrupted();
        reset_orbit_delay();
        m_main_loop_scope.emplace(std::make_shared<MainLoopKeyboardHandler>());
        m_orbit_toggle_scope.emplace(std::make_shared<OrbitToggleKeyboardHandler>(m_calculation));
        if (const std::string diagnostic{m_calculation.prepare_palette()}; !diagnostic.empty())
        {
            stop_msg(diagnostic);
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

void FrothyBasinController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The FrothyBasin controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        if (!m_sound.wait_until_ready())
        {
            suspend();
            return;
        }
        bool check_interrupted{true};
        m_calculation.iterate();
        display_disk_storage_progress();
        if (m_calculation.consume_orbit_overlay_displayed())
        {
            pace_orbit_display();
            check_interrupted = false;
        }
        if (m_calculation.done())
        {
            finish();
        }
        else if (check_interrupted ? calc_interrupted() : calc_interrupt_requested())
        {
            suspend();
        }
    }
    catch (...)
    {
        abort();
        throw;
    }
}

bool FrothyBasinController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int FrothyBasinController::result() const
{
    if (!done())
    {
        throw std::logic_error("The FrothyBasin controller has not completed");
    }
    return m_result;
}

void FrothyBasinController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active FrothyBasin controller");
    }
    try
    {
        m_sound.cancel();
        m_calculation.scrub_overlay();
        m_calculation.suspend();
        close(-1);
    }
    catch (...)
    {
        abort();
        throw;
    }
}

void FrothyBasinController::finish()
{
    close(m_sound.finish() ? 0 : -1);
}

void FrothyBasinController::abort()
{
    if (done())
    {
        return;
    }
    close(-1);
    m_calculation.scrub_overlay();
    m_calculation.finish(true);
    m_sound.cancel();
}

void FrothyBasinController::close(const int result)
{
    m_orbit_toggle_scope.reset();
    m_main_loop_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
