#include "intro.h"

/*
 * intro screen (authors & credits)
 */
#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "engine_timer.h"
#include "help_title.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "put_string_center.h"
#include "value_saver.h"

#include <cstdlib>
#include <ctime>
#include <vector>

bool g_slow_display{};

void intro()
{
    // following overlayed data safe if "putstrings" are resident
    static char PRESS_ENTER[] = {"Press ENTER for main menu, F1 for help."};
    std::vector<int> authors;
    char credits[32768] = { 0 };
    char screen_text[32768];

    g_timer_start -= std::clock();       // "time out" during help
    ValueSaver saved_look_at_mouse{g_look_at_mouse, +MouseLook::IGNORE};
    ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_MENU};

    int i = 32767 + read_help_topic(help_labels::INTRO_AUTHORS, 0, 32767, screen_text);
    screen_text[i] = '\0';
    i = 32767 + read_help_topic(help_labels::INTRO_CREDITS, 0, 32767, credits);
    credits[i] = '\0';

    int j = 0;
    authors.push_back(0);               // find the start of each credit-line
    for (i = 0; credits[i] != 0; i++)
    {
        if (credits[i] == '\n')
        {
            authors.push_back(i+1);
        }
    }
    authors.push_back(i);

    helptitle();
#define END_MAIN_AUTHOR 6
    int toprow = END_MAIN_AUTHOR + 1;
    int botrow = 21;
    putstringcenter(1, 0, 80, C_TITLE, PRESS_ENTER);
    driver_put_string(2, 0, C_CONTRIB, screen_text);
    driver_set_attr(2, 0, C_AUTHDIV1, 80);
    driver_set_attr(3, 0, C_PRIMARY, 80);
    driver_set_attr(4, 0, C_AUTHDIV1, 80);
    driver_set_attr(5, 0, C_CONTRIB, 80);
    driver_set_attr(END_MAIN_AUTHOR, 0, C_AUTHDIV1, 80);
    driver_set_attr(22, 0, C_AUTHDIV2, 80);
    driver_set_attr(23, 0, C_TITLE_LOW, 160);

    driver_set_attr(toprow, 0, C_CONTRIB, (21-END_MAIN_AUTHOR)*80);
    srand((unsigned int)std::clock());
    j = std::rand()%(j-(botrow-toprow)); // first to use
    i = j+botrow-toprow; // last to use
    char oldchar = credits[authors.at(i + 1)];
    credits[authors.at(i+1)] = 0;
    driver_put_string(toprow, 0, C_CONTRIB, credits+authors.at(j));
    credits[authors.at(i+1)] = oldchar;
    int delaymax = 10;
    driver_hide_text_cursor();
    while (! driver_key_pressed())
    {
        if (g_slow_display)
        {
            delaymax *= 15;
        }
        for (j = 0; j < delaymax && !(driver_key_pressed()); j++)
        {
            driver_delay(100);
        }
        if (driver_key_pressed() == ID_KEY_SPACE)
        {
            // spacebar pauses
            driver_get_key();
            driver_wait_key_pressed(0);
            if (driver_key_pressed() == ID_KEY_SPACE)
            {
                driver_get_key();
            }
        }
        delaymax = 15;
        driver_scroll_up(toprow, botrow);
        i++;
        if (credits[authors.at(i)] == 0)
        {
            i = 0;
        }
        oldchar = credits[authors.at(i+1)];
        credits[authors.at(i+1)] = 0;
        driver_put_string(botrow, 0, C_CONTRIB, &credits[authors.at(i)]);
        driver_set_attr(botrow, 0, C_CONTRIB, 80);
        credits[authors.at(i+1)] = oldchar;
        driver_hide_text_cursor(); // turn it off
    }
}
