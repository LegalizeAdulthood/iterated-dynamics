#include "intro.h"

/*
 * intro screen (authors & credits)
 */
#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "engine_timer.h"
#include "helpcom.h"
#include "helpdefs.h"
#include "help_title.h"
#include "id_data.h"
#include "id_keys.h"
#include "put_string_center.h"

#include <ctime>
#include <vector>

bool slowdisplay = false;

void intro()
{
    // following overlayed data safe if "putstrings" are resident
    static char PRESS_ENTER[] = {"Press ENTER for main menu, F1 for help."};
    int       toprow, botrow, delaymax;
    char      oldchar;
    std::vector<int> authors;
    char credits[32768] = { 0 };
    char screen_text[32768];
    int old_look_at_mouse;

    g_timer_start -= std::clock();       // "time out" during help
    old_look_at_mouse = g_look_at_mouse;
    help_labels const old_help_mode = g_help_mode;
    g_look_at_mouse = 0;                    // de-activate full mouse checking

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
#define END_MAIN_AUTHOR 4
    toprow = END_MAIN_AUTHOR+1;
    botrow = 21;
    putstringcenter(1, 0, 80, C_TITLE, PRESS_ENTER);
    driver_put_string(2, 0, C_CONTRIB, screen_text);
    driver_set_attr(3, 0, C_PRIMARY, 80*(END_MAIN_AUTHOR-3));
    driver_set_attr(2, 0, C_AUTHDIV1, 80);
    driver_set_attr(END_MAIN_AUTHOR, 0, C_AUTHDIV1, 80);
    driver_set_attr(22, 0, C_AUTHDIV2, 80);
    driver_set_attr(23, 0, C_TITLE_LOW, 160);

    driver_set_attr(toprow, 0, C_CONTRIB, (21-END_MAIN_AUTHOR)*80);
    srand((unsigned int)std::clock());
    j = rand()%(j-(botrow-toprow)); // first to use
    i = j+botrow-toprow; // last to use
    oldchar = credits[authors.at(i+1)];
    credits[authors.at(i+1)] = 0;
    driver_put_string(toprow, 0, C_CONTRIB, credits+authors.at(j));
    credits[authors.at(i+1)] = oldchar;
    delaymax = 10;
    driver_hide_text_cursor();
    g_help_mode = help_labels::HELPMENU;
    while (! driver_key_pressed())
    {
        if (slowdisplay)
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

    g_look_at_mouse = old_look_at_mouse;                // restore the mouse-checking
    g_help_mode = old_help_mode;
}
