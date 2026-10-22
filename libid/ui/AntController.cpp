// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/AntController.h"

#include "engine/NonStandardFractal.h"
#include "engine/OrbitPlot.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"
#include "ui/id_keys.h"
#include "ui/main_menu_switch.h"
#include "ui/temp_msg.h"

#include <helpdefs.h>

#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <stdexcept>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{
namespace
{

using Clock = std::chrono::steady_clock;

constexpr std::chrono::milliseconds ANT_DELAY_SLEEP_CHUNK{1};

int normalize_menu_key(const int key)
{
    if (key >= 'A' && key <= 'Z')
    {
        return key - 'A' + 'a';
    }
    return key;
}

bool key_opens_options_menu(const int key)
{
    switch (normalize_menu_key(key))
    {
    case ID_KEY_CTL_B:
    case ID_KEY_CTL_E:
    case ID_KEY_CTL_F:
    case 'g':
    case 'p':
    case 'v':
    case 'x':
    case 'y':
    case 'z':
        return true;

    default:
        return false;
    }
}

bool run_options_menu(const int key)
{
    MainContext context;
    context.key = normalize_menu_key(key);
    context.more_keys = true;

    const CalcStatus saved_status{g_calc_status};
    const CommandResult result{main_menu_switch(context)};
    return result == CommandResult::FRACTAL_TYPE_CHANGED || result == CommandResult::RENDER_REQUESTED ||
        result == CommandResult::RESTORE_REQUESTED || result == CommandResult::FULL_RESET_REQUESTED ||
        result == CommandResult::EXIT_REQUESTED || !context.more_keys || g_calc_status != saved_status;
}

bool stop_ant(const int key, const bool key_peeked)
{
    g_calc_status = CalcStatus::NON_RESUMABLE;
    if (!key_peeked)
    {
        driver_unget_key(key);
    }
    return true;
}

} // namespace

AntController::AntController(NonStandardFractal &calculation) :
    m_calculation{calculation}
{
}

void AntController::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The Ant controller has already started");
    }
    m_help_scope.emplace(g_help_mode, HelpLabels::HELP_ANT_COMMANDS);
    m_wait = std::abs(g_orbit_delay);
    m_step = m_wait == 1;
    if (m_step)
    {
        m_wait = 0;
    }
    m_phase = Phase::ACTIVE;
    if (m_calculation.done())
    {
        close(0);
    }
}

void AntController::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The Ant controller has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        m_calculation.iterate();
        const bool batch_complete{m_calculation.consume_batch_complete()};
        if (batch_complete)
        {
            display_disk_storage_progress();
        }
        if (m_calculation.done())
        {
            close(0);
            return;
        }
        if (!batch_complete)
        {
            return;
        }
        if (m_wait > 0 && !m_step)
        {
            wait_for_delay();
        }
        if (m_step)
        {
            driver_flush();
            driver_delay(0);
        }
        if (process_keyboard())
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

bool AntController::done() const
{
    return m_phase == Phase::COMPLETE;
}

int AntController::result() const
{
    if (!done())
    {
        throw std::logic_error("The Ant controller has not completed");
    }
    return m_result;
}

void AntController::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active Ant controller");
    }
    m_calculation.suspend();
    close(0);
}

void AntController::wait_for_delay()
{
    const Clock::time_point deadline{Clock::now() + std::chrono::microseconds{m_wait * 100}};
    driver_flush();
    Clock::time_point now{Clock::now()};
    while (now < deadline)
    {
        const Clock::duration remaining{deadline - now};
        if (remaining > ANT_DELAY_SLEEP_CHUNK)
        {
            driver_delay(static_cast<int>(ANT_DELAY_SLEEP_CHUNK.count()));
        }
        else
        {
            driver_delay(0);
        }
        now = Clock::now();
    }
}

void AntController::change_wait()
{
    while (true)
    {
        show_temp_msg(fmt::format("Delay {:4}     ", m_wait));
        switch (driver_get_key())
        {
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
            m_wait += 100;
            break;
        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
            m_wait += 10;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            m_wait -= 100;
            break;
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_DOWN_ARROW:
            m_wait -= 10;
            break;
        default:
            clear_temp_msg();
            return;
        }
        m_wait = std::max(m_wait, 0L);
    }
}

bool AntController::process_keyboard()
{
    int key = driver_key_pressed();
    const bool key_peeked{key != 0};
    if (key || m_step)
    {
        if (key == 0)
        {
            key = driver_get_key();
        }
        switch (key)
        {
        case ID_KEY_SPACE:
            m_step = !m_step;
            break;
        case ID_KEY_ESC:
            return stop_ant(key, key_peeked);

        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
        case ID_KEY_DOWN_ARROW:
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            change_wait();
            break;

        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            break;

        default:
            if (key_opens_options_menu(key))
            {
                if (key_peeked)
                {
                    driver_get_key();
                }
                return run_options_menu(key);
            }
            return stop_ant(key, key_peeked);
        }
        if (key_peeked)
        {
            driver_get_key();
        }
    }
    return false;
}

void AntController::close(const int result)
{
    m_help_scope.reset();
    m_result = result;
    m_phase = Phase::COMPLETE;
}

} // namespace id::ui
