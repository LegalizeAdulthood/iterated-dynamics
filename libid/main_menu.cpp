// SPDX-License-Identifier: GPL-3.0-only
//
#include "main_menu.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "full_screen_choice.h"
#include "id.h"
#include "id_data.h"
#ifdef XFRACT
#include "os.h"
#endif
#include "goodbye.h"
#include "help_title.h"
#include "id_keys.h"
#include "put_string_center.h"
#include "rotate.h"
#include "tab_display.h"
#include "video_mode.h"

#include <cstring>
#include <helpcom.h>

enum
{
    MENU_HDG = 3,
    MENU_ITEM = 1
};

static bool s_full_menu{};

static int menu_checkkey(int curkey, int /*choice*/)
{
    int testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
    // We use F2 for shift-@, annoyingly enough
    if (testkey == ID_KEY_F2)
    {
        return -testkey;
    }
#endif
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
    if (check_vidmode_key(0, testkey) >= 0)
    {
        return -testkey;
    }
    return 0;
}

int main_menu(bool full_menu)
{
    char const *choices[44]; // 2 columns * 22 rows
    int attributes[44];
    int choicekey[44];
    int i;
    int nextleft;
    int nextright;
    bool showjuliatoggle;
    bool const old_tab_mode = g_tab_mode;

top:
    s_full_menu = full_menu;
    g_tab_mode = false;
    showjuliatoggle = false;
    for (int j = 0; j < 44; ++j)
    {
        attributes[j] = 256;
        choices[j] = "";
        choicekey[j] = -1;
    }
    nextleft = -2;
    nextright = -1;

    if (full_menu)
    {
        nextleft += 2;
        choices[nextleft] = "      CURRENT IMAGE         ";
        attributes[nextleft] = 256+MENU_HDG;

        nextleft += 2;
        choicekey[nextleft] = 13; // enter
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = (g_calc_status == calc_status_value::RESUMABLE) ?
                            "Continue calculation        " :
                            "Return to image             ";

        nextleft += 2;
        choicekey[nextleft] = 9; // tab
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Info about image      <Tab> ";

        nextleft += 2;
        choicekey[nextleft] = 'o';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Orbits window          <O>  ";
        if (!(g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::INVERSEJULIA))
        {
            nextleft += 2;
        }
    }

    nextleft += 2;
    choices[nextleft] = "      NEW IMAGE             ";
    attributes[nextleft] = 256+MENU_HDG;

    nextleft += 2;
    choicekey[nextleft] = ID_KEY_DELETE;
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Select video mode...  <Del> ";

    nextleft += 2;
    choicekey[nextleft] = 't';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Select fractal type    <T>  ";

    if (full_menu)
    {
        if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
            || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
        {
            nextleft += 2;
            choicekey[nextleft] = ID_KEY_SPACE;
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "Toggle to/from Julia <Space>";
            showjuliatoggle = true;
        }
        if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP
            || g_fractal_type == fractal_type::INVERSEJULIA)
        {
            nextleft += 2;
            choicekey[nextleft] = 'j';
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "Toggle to/from inverse <J>  ";
            showjuliatoggle = true;
        }

        nextleft += 2;
        choicekey[nextleft] = 'h';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Return to prior image  <H>   ";

        nextleft += 2;
        choicekey[nextleft] = ID_KEY_BACKSPACE;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Reverse thru history <Ctrl+H>";
    }
    else
    {
        nextleft += 2;
    }

    nextleft += 2;
    choices[nextleft] = "      OPTIONS                ";
    attributes[nextleft] = 256+MENU_HDG;

    nextleft += 2;
    choicekey[nextleft] = 'x';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Basic options...       <X>  ";

    nextleft += 2;
    choicekey[nextleft] = 'y';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Extended options...    <Y>  ";

    nextleft += 2;
    choicekey[nextleft] = 'z';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Type-specific parms... <Z>  ";

    nextleft += 2;
    choicekey[nextleft] = 'p';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Passes options...      <P>  ";

    nextleft += 2;
    choicekey[nextleft] = 'v';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "View window options... <V>  ";

    if (!showjuliatoggle)
    {
        nextleft += 2;
        choicekey[nextleft] = 'i';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Fractal 3D parms...    <I>  ";
    }

    nextleft += 2;
    choicekey[nextleft] = ID_KEY_CTL_B;
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "Browse params...    <Ctrl+B>";

    if (full_menu)
    {
        nextleft += 2;
        choicekey[nextleft] = ID_KEY_CTL_E;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Evolver params...   <Ctrl+E>";

        nextleft += 2;
        choicekey[nextleft] = ID_KEY_CTL_F;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "Sound params...     <Ctrl+F>";
    }

    nextright += 2;
    attributes[nextright] = 256 + MENU_HDG;
    choices[nextright] = "        FILE                  ";

    nextright += 2;
    choicekey[nextright] = '@';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Run saved command set... <@>  ";

    if (full_menu)
    {
        nextright += 2;
        choicekey[nextright] = 's';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "Save image to file       <S>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'r';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Load image from file...  <R>  ";

    nextright += 2;
    choicekey[nextright] = '3';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "3D transform from file...<3>  ";

    if (full_menu)
    {
        nextright += 2;
        choicekey[nextright] = '#';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "3D overlay from file.....<#>  ";

        nextright += 2;
        choicekey[nextright] = 'b';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "Save current parameters..<B>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'd';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Command shell            <D>  ";

    nextright += 2;
    choicekey[nextright] = 'g';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Give parameter string    <G>  ";

    nextright += 2;
    choicekey[nextright] = ID_KEY_ESC;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Quit Id                  <Esc> ";

    nextright += 2;
    choicekey[nextright] = ID_KEY_INSERT;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Restart Id               <Ins> ";

#ifdef XFRACT
    if (full_menu && (g_got_real_dac || g_fake_lut) && g_colors >= 16)
#else
    if (full_menu && g_got_real_dac && g_colors >= 16)
#endif
    {
        nextright += 2;
        choices[nextright] = "       COLORS                 ";
        attributes[nextright] = 256+MENU_HDG;

        nextright += 2;
        choicekey[nextright] = 'c';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "Color cycling mode       <C>  ";

        nextright += 2;
        choicekey[nextright] = '+';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "Rotate palette      <+>, <->  ";

        if (g_colors > 16)
        {
            nextright += 2;
            choicekey[nextright] = 'e';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "Palette editing mode     <E>  ";

            nextright += 2;
            choicekey[nextright] = 'a';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "Make starfield           <A>  ";
        }
    }

    nextright += 2;
    choicekey[nextright] = ID_KEY_CTL_A;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Ant automaton         <Ctrl+A>";

    nextright += 2;
    choicekey[nextright] = ID_KEY_CTL_S;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "Stereogram            <Ctrl+S>";

    i = driver_key_pressed() ? driver_get_key() : 0;
    if (menu_checkkey(i, 0) == 0)
    {
        g_help_mode = help_labels::HELP_MAIN;         // switch help modes
        nextleft += 2;
        if (nextleft < nextright)
        {
            nextleft = nextright + 1;
        }
        i = full_screen_choice(CHOICE_MENU | CHOICE_CRUNCH, "MAIN MENU", nullptr, nullptr, nextleft,
            choices, attributes, 2, nextleft / 2, 29, 0, nullptr, nullptr, nullptr, menu_checkkey);
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
            i = choicekey[i];
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
        putstringcenter(10, 18, 40, C_GENERAL_INPUT,
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
    g_tab_mode = old_tab_mode;
    return i;
}
