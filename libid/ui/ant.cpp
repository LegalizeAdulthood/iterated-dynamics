// SPDX-License-Identifier: GPL-3.0-only
//
// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "ui/ant.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/Ant.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/id_keys.h"
#include "ui/temp_msg.h"

#include <sstream>

using namespace id;
using namespace id::misc;

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
            return true;

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
            break;

        default:
            return true;
        }
        if (driver_key_pressed())
        {
            driver_get_key();
        }
    }
    return false;
}

int ant_type()
{
    ValueSaver saved_help_mode(g_help_mode, id::help::HelpLabels::HELP_ANT_COMMANDS);
    id::fractals::Ant ant;
    long wait = std::abs(g_orbit_delay);

    bool step = wait == 1;
    if (step)
    {
        wait = 0;
    }
    while (ant.iterate(step, wait))
    {
        if (ant_key(step, wait))
        {
            break;
        }
    }
    return 0;
}
