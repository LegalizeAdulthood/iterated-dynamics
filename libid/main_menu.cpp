// SPDX-License-Identifier: GPL-3.0-only
//
#include "main_menu.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "full_screen_choice.h"
#include "goodbye.h"
#include "help_title.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "put_string_center.h"
#include "rotate.h"
#include "tab_display.h"
#include "value_saver.h"
#include "video_mode.h"

#include <cstring>
#include <helpcom.h>

enum
{
    MENU_HDG = 3,
    MENU_ITEM = 1
};

static bool s_full_menu{};

static int menu_check_key(int curkey, int /*choice*/)
{
    int testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey+('a'-'A') : curkey;
    if (testkey == '2')
    {
        testkey = '@';
    }
    if (std::strchr("#@2txyzgvir3dj", testkey)
        || testkey == ID_KEY_INSERT || testkey == ID_KEY_CTL_B
        || testkey == ID_KEY_ESC || testkey == ID_KEY_DELETE
        || testkey == ID_KEY_CTL_F)
    {
        return -testkey;
    }
    if (s_full_menu)
    {
        if (std::strchr("\\sobkrh", testkey)
            || testkey == ID_KEY_TAB || testkey == ID_KEY_CTL_A
            || testkey == ID_KEY_CTL_E || testkey == ID_KEY_BACKSPACE
            || testkey == ID_KEY_CTL_S || testkey == ID_KEY_CTL_U)   // Ctrl+A, E, H, S, U
        {
            return -testkey;
        }
        if (testkey == ' ')
        {
            if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
                || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
            {
                return -testkey;
            }
        }
        if (g_got_real_dac && g_colors >= 16)
        {
            if (std::strchr("c+-", testkey))
            {
                return -testkey;
            }
            if (g_colors > 16 && (testkey == 'a' || (testkey == 'e')))
            {
                return -testkey;
            }
        }
        // Alt+A and Alt+S
        if (testkey == ID_KEY_ALT_A || testkey == ID_KEY_ALT_S)
        {
            return -testkey;
        }
    }
    if (check_vid_mode_key(0, testkey) >= 0)
    {
        return -testkey;
    }
    return 0;
}

int main_menu(bool full_menu)
{
    char const *choices[44]; // 2 columns * 22 rows
    int attributes[44];
    int choice_key[44];
    ValueSaver saved_tab_mode{g_tab_mode};

top:
    s_full_menu = full_menu;
    g_tab_mode = false;
    bool show_julia_toggle{};
    for (int j = 0; j < 44; ++j)
    {
        attributes[j] = 256;
        choices[j] = "";
        choice_key[j] = -1;
    }
    int next_left = -2;
    int next_right = -1;

    if (full_menu)
    {
        next_left += 2;
        choices[next_left] = "      CURRENT IMAGE         ";
        attributes[next_left] = 256+MENU_HDG;

        next_left += 2;
        choice_key[next_left] = 13; // enter
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = (g_calc_status == calc_status_value::RESUMABLE) ?
                            "Continue calculation        " :
                            "Return to image             ";

        next_left += 2;
        choice_key[next_left] = 9; // tab
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Info about image      <Tab> ";

        next_left += 2;
        choice_key[next_left] = 'o';
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Orbits window          <O>  ";
        if (!(g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::INVERSEJULIA))
        {
            next_left += 2;
        }
    }

    next_left += 2;
    choices[next_left] = "      NEW IMAGE             ";
    attributes[next_left] = 256+MENU_HDG;

    next_left += 2;
    choice_key[next_left] = ID_KEY_DELETE;
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Select video mode...  <Del> ";

    next_left += 2;
    choice_key[next_left] = 't';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Select fractal type    <T>  ";

    if (full_menu)
    {
        if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
            || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
        {
            next_left += 2;
            choice_key[next_left] = ID_KEY_SPACE;
            attributes[next_left] = MENU_ITEM;
            choices[next_left] = "Toggle to/from Julia <Space>";
            show_julia_toggle = true;
        }
        if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP
            || g_fractal_type == fractal_type::INVERSEJULIA)
        {
            next_left += 2;
            choice_key[next_left] = 'j';
            attributes[next_left] = MENU_ITEM;
            choices[next_left] = "Toggle to/from inverse <J>  ";
            show_julia_toggle = true;
        }

        next_left += 2;
        choice_key[next_left] = 'h';
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Return to prior image  <H>   ";

        next_left += 2;
        choice_key[next_left] = ID_KEY_BACKSPACE;
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Reverse thru history <Ctrl+H>";
    }
    else
    {
        next_left += 2;
    }

    next_left += 2;
    choices[next_left] = "      OPTIONS                ";
    attributes[next_left] = 256+MENU_HDG;

    next_left += 2;
    choice_key[next_left] = 'x';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Basic options...       <X>  ";

    next_left += 2;
    choice_key[next_left] = 'y';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Extended options...    <Y>  ";

    next_left += 2;
    choice_key[next_left] = 'z';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Type-specific parms... <Z>  ";

    next_left += 2;
    choice_key[next_left] = 'p';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Passes options...      <P>  ";

    next_left += 2;
    choice_key[next_left] = 'v';
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "View window options... <V>  ";

    if (!show_julia_toggle)
    {
        next_left += 2;
        choice_key[next_left] = 'i';
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Fractal 3D parms...    <I>  ";
    }

    next_left += 2;
    choice_key[next_left] = ID_KEY_CTL_B;
    attributes[next_left] = MENU_ITEM;
    choices[next_left] = "Browse params...    <Ctrl+B>";

    if (full_menu)
    {
        next_left += 2;
        choice_key[next_left] = ID_KEY_CTL_E;
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Evolver params...   <Ctrl+E>";

        next_left += 2;
        choice_key[next_left] = ID_KEY_CTL_F;
        attributes[next_left] = MENU_ITEM;
        choices[next_left] = "Sound params...     <Ctrl+F>";
    }

    next_right += 2;
    attributes[next_right] = 256 + MENU_HDG;
    choices[next_right] = "        FILE                  ";

    next_right += 2;
    choice_key[next_right] = '@';
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Run saved command set... <@>  ";

    if (full_menu)
    {
        next_right += 2;
        choice_key[next_right] = 's';
        attributes[next_right] = MENU_ITEM;
        choices[next_right] = "Save image to file       <S>  ";
    }

    next_right += 2;
    choice_key[next_right] = 'r';
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Load image from file...  <R>  ";

    next_right += 2;
    choice_key[next_right] = '3';
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "3D transform from file...<3>  ";

    if (full_menu)
    {
        next_right += 2;
        choice_key[next_right] = '#';
        attributes[next_right] = MENU_ITEM;
        choices[next_right] = "3D overlay from file.....<#>  ";

        next_right += 2;
        choice_key[next_right] = 'b';
        attributes[next_right] = MENU_ITEM;
        choices[next_right] = "Save current parameters..<B>  ";
    }

    next_right += 2;
    choice_key[next_right] = 'd';
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Command shell            <D>  ";

    next_right += 2;
    choice_key[next_right] = 'g';
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Give parameter string    <G>  ";

    next_right += 2;
    choice_key[next_right] = ID_KEY_ESC;
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Quit Id                  <Esc> ";

    next_right += 2;
    choice_key[next_right] = ID_KEY_INSERT;
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Restart Id               <Ins> ";

    if (full_menu && g_got_real_dac && g_colors >= 16)
    {
        next_right += 2;
        choices[next_right] = "       COLORS                 ";
        attributes[next_right] = 256+MENU_HDG;

        next_right += 2;
        choice_key[next_right] = 'c';
        attributes[next_right] = MENU_ITEM;
        choices[next_right] = "Color cycling mode       <C>  ";

        next_right += 2;
        choice_key[next_right] = '+';
        attributes[next_right] = MENU_ITEM;
        choices[next_right] = "Rotate palette      <+>, <->  ";

        if (g_colors > 16)
        {
            next_right += 2;
            choice_key[next_right] = 'e';
            attributes[next_right] = MENU_ITEM;
            choices[next_right] = "Palette editing mode     <E>  ";

            next_right += 2;
            choice_key[next_right] = 'a';
            attributes[next_right] = MENU_ITEM;
            choices[next_right] = "Make starfield           <A>  ";
        }
    }

    next_right += 2;
    choice_key[next_right] = ID_KEY_CTL_A;
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Ant automaton         <Ctrl+A>";

    next_right += 2;
    choice_key[next_right] = ID_KEY_CTL_S;
    attributes[next_right] = MENU_ITEM;
    choices[next_right] = "Stereogram            <Ctrl+S>";

    int i = driver_key_pressed() ? driver_get_key() : 0;
    if (menu_check_key(i, 0) == 0)
    {
        g_help_mode = help_labels::HELP_MAIN;         // switch help modes
        next_left += 2;
        if (next_left < next_right)
        {
            next_left = next_right + 1;
        }
        i = full_screen_choice(CHOICE_MENU | CHOICE_CRUNCH, "MAIN MENU", nullptr, nullptr, next_left,
            choices, attributes, 2, next_left / 2, 29, 0, nullptr, nullptr, nullptr, menu_check_key);
        if (i == -1)     // escape
        {
            i = ID_KEY_ESC;
        }
        else if (i < 0)
        {
            i = 0 - i;
        }
        else                      // user selected a choice
        {
            i = choice_key[i];
            if (-10 == i)
            {
                g_help_mode = help_labels::HELP_ZOOM;
                help();
                i = 0;
            }
        }
    }
    if (i == ID_KEY_ESC)             // escape from menu exits
    {
        help_title();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
        for (int j = 9; j <= 11; ++j)
        {
            driver_set_attr(j, 18, C_GENERAL_INPUT, 40);
        }
        put_string_center(10, 18, 40, C_GENERAL_INPUT,
                        "Exit from " ID_PROGRAM_NAME " (y/n)? y"
                       );
        driver_hide_text_cursor();
        while ((i = driver_get_key()) != 'y' && i != 'Y' && i != ID_KEY_ENTER)
        {
            if (i == 'n' || i == 'N')
            {
                goto top;
            }
        }
        goodbye();
    }
    if (i == ID_KEY_TAB)
    {
        tab_display();
        i = 0;
    }
    if (i == ID_KEY_ENTER || i == ID_KEY_ENTER_2)
    {
        i = 0;                 // don't trigger new calc
    }
    return i;
}
