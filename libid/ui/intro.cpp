// SPDX-License-Identifier: GPL-3.0-only
//
/*
 * intro screen (authors & credits)
 */
#include "ui/intro.h"

#include "engine/cmdfiles.h"
#include "engine/engine_timer.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/help.h"
#include "ui/help_title.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/put_string_center.h"

#include <cstdlib>
#include <ctime>
#include <vector>

using namespace id::engine;
using namespace id::help;
using namespace id::misc;

namespace id::ui
{

bool g_slow_display{};

static std::vector<int> get_authors(const char *credits)
{
    std::vector<int> authors;
    authors.push_back(0); // find the start of each credit-line
    int i;
    for (i = 0; credits[i] != 0; i++)
    {
        if (credits[i] == '\n')
        {
            authors.push_back(i + 1);
        }
    }
    authors.push_back(i);
    return authors;
}

void intro()
{
    char credits[32768]{};
    char screen_text[32768];

    g_engine_timer_start -= std::clock();       // "time out" during help
    ValueSaver saved_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_MENU};

    const auto read_topic = [](HelpLabels label, char buffer[32768])
    {
        const int i = 32767 + read_help_topic(label, 0, 32767, buffer);
        buffer[i] = '\0';
    };
    read_topic(HelpLabels::INTRO_AUTHORS, screen_text);
    read_topic(HelpLabels::INTRO_CREDITS, credits);

    int j = 0;
    const std::vector authors{get_authors(credits)};

    help_title();
    constexpr int END_MAIN_AUTHOR{6};
    constexpr int top_row = END_MAIN_AUTHOR + 1;
    int bot_row = 21;
    put_string_center(1, 0, 80, C_TITLE, "Press ENTER for main menu, F1 for help.");
    driver_put_string(2, 0, C_CONTRIB, screen_text);
    driver_set_attr(2, 0, C_AUTH_DIV1, 80);
    driver_set_attr(3, 0, C_PRIMARY, 80);
    driver_set_attr(4, 0, C_AUTH_DIV1, 80);
    driver_set_attr(5, 0, C_CONTRIB, 80);
    driver_set_attr(END_MAIN_AUTHOR, 0, C_AUTH_DIV1, 80);
    driver_set_attr(22, 0, C_AUTH_DIV2, 80);
    driver_set_attr(23, 0, C_TITLE_LOW, 160);

    driver_set_attr(top_row, 0, C_CONTRIB, (21-END_MAIN_AUTHOR)*80);
    std::srand(static_cast<unsigned int>(std::clock()));
    j = std::rand()%(j-(bot_row-top_row)); // first to use
    int i = j+bot_row-top_row; // last to use
    char old_char = credits[authors.at(i + 1)];
    credits[authors.at(i+1)] = 0;
    driver_put_string(top_row, 0, C_CONTRIB, credits+authors.at(j));
    credits[authors.at(i+1)] = old_char;
    int delay_max = 10;
    driver_hide_text_cursor();
    while (! driver_key_pressed())
    {
        if (g_slow_display)
        {
            delay_max *= 15;
        }
        for (j = 0; j < delay_max && !driver_key_pressed(); j++)
        {
            driver_delay(100);
        }
        if (driver_key_pressed() == ID_KEY_SPACE)
        {
            // spacebar pauses
            driver_get_key();
            driver_wait_key_pressed(false);
            if (driver_key_pressed() == ID_KEY_SPACE)
            {
                driver_get_key();
            }
        }
        delay_max = 15;
        driver_scroll_up(top_row, bot_row);
        i++;
        if (credits[authors.at(i)] == 0)
        {
            i = 0;
        }
        old_char = credits[authors.at(i+1)];
        credits[authors.at(i+1)] = 0;
        driver_put_string(bot_row, 0, C_CONTRIB, &credits[authors.at(i)]);
        driver_set_attr(bot_row, 0, C_CONTRIB, 80);
        credits[authors.at(i+1)] = old_char;
        driver_hide_text_cursor(); // turn it off
    }
}

} // namespace id::ui
