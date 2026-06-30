// SPDX-License-Identifier: GPL-3.0-only
//
// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "ui/ant.h"

#include "engine/calcfrac.h"
#include "engine/orbit.h"
#include "fractals/Ant.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/help.h"
#include "ui/id_keys.h"
#include "ui/main_menu_switch.h"
#include "ui/temp_msg.h"

#include <chrono>
#include <sstream>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

namespace
{

using Clock = std::chrono::steady_clock;

constexpr auto ANT_DELAY_SLEEP_CHUNK = std::chrono::milliseconds{1};

void wait_for_ant_delay(const long delay_100us)
{
    const auto deadline = Clock::now() + std::chrono::microseconds{delay_100us * 100};
    driver_flush();
    auto now = Clock::now();
    while (now < deadline)
    {
        const auto remaining = deadline - now;
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
    const MainState state{main_menu_switch(context)};
    return state == MainState::IMAGE_START || state == MainState::RESTORE_START || state == MainState::RESTART ||
        !context.more_keys || g_calc_status != saved_status;
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

static long change_wait(long wait)
{
    while (true)
    {
        std::ostringstream msg;
        msg << "Delay " << std::setw(4) << wait << "     ";
        show_temp_msg(msg.str());
        switch (driver_get_key())
        {
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
            wait += 100;
            break;
        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
            wait += 10;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            wait -= 100;
            break;
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_DOWN_ARROW:
            wait -= 10;
            break;
        default:
            clear_temp_msg();
            return wait;
        }
        wait = std::max(wait, 0L);
    }
}

static bool ant_key(bool &step, long &wait)
{
    // check for a key only every inner_loop times
    int key = driver_key_pressed();
    const bool key_peeked{key != 0};
    if (key || step)
    {
        if (key == 0)
        {
            key = driver_get_key();
        }
        switch (key)
        {
        case ID_KEY_SPACE:
            step = !step;
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
            wait = change_wait(wait);
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

int ant_type()
{
    ValueSaver saved_help_mode(g_help_mode, HelpLabels::HELP_ANT_COMMANDS);
    fractals::Ant ant;
    long wait = std::abs(g_orbit_delay);

    bool step = wait == 1;
    if (step)
    {
        wait = 0;
    }
    while (!ant.done())
    {
        ant.iterate();
        const bool batch_complete{ant.consume_batch_complete()};
        if (!ant.done() && batch_complete && wait > 0 && !step)
        {
            wait_for_ant_delay(wait);
        }
        if (!ant.done() && batch_complete && step)
        {
            driver_flush();
            driver_delay(0);
        }
        if (!ant.done() && batch_complete && ant_key(step, wait))
        {
            break;
        }
    }
    return 0;
}

} // namespace id::ui
