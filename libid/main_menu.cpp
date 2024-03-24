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
#include "help_title.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "video_mode.h"

#include <cstring>
#include <helpcom.h>

static bool s_full_menu{};

static int menu_checkkey(int curkey, int /*choice*/)
{
    int testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
    // We use F2 for shift-@, annoyingly enough
    if (testkey == FIK_F2)
    {
        return -testkey;
    }
#endif
    if (testkey == '2')
    {
        testkey = '@';

    }
    if (std::strchr("#@2txyzgvir3dj", testkey)
        || testkey == FIK_INSERT || testkey == FIK_CTL_B
        || testkey == FIK_ESC || testkey == FIK_DELETE
        || testkey == FIK_CTL_F)
    {
        return -testkey;
    }
    if (s_full_menu)
    {
        if (std::strchr("\\sobpkrh", testkey)
            || testkey == FIK_TAB || testkey == FIK_CTL_A
            || testkey == FIK_CTL_E || testkey == FIK_BACKSPACE
            || testkey == FIK_CTL_P || testkey == FIK_CTL_S
            || testkey == FIK_CTL_U)   // ctrl-A, E, H, P, S, U
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
        // Alt-A and Alt-S
        if (testkey == FIK_ALT_A || testkey == FIK_ALT_S)
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

#define MENU_HDG 3
#define MENU_ITEM 1

int main_menu(bool full_menu)
{
    char const *choices[44]; // 2 columns * 22 rows
    int attributes[44];
    int choicekey[44];
    int i;
    int nextleft, nextright;
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
                            "continue calculation        " :
                            "return to image             ";

        nextleft += 2;
        choicekey[nextleft] = 9; // tab
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "info about image      <tab> ";

        nextleft += 2;
        choicekey[nextleft] = 'o';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "orbits window          <o>  ";
        if (!(g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP || g_fractal_type == fractal_type::INVERSEJULIA))
        {
            nextleft += 2;
        }
    }

    nextleft += 2;
    choices[nextleft] = "      NEW IMAGE             ";
    attributes[nextleft] = 256+MENU_HDG;

    nextleft += 2;
    choicekey[nextleft] = FIK_DELETE;
    attributes[nextleft] = MENU_ITEM;
#ifdef XFRACT
    choices[nextleft] = "draw fractal           <D>  ";
#else
    choices[nextleft] = "select video mode...  <del> ";
#endif

    nextleft += 2;
    choicekey[nextleft] = 't';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "select fractal type    <t>  ";

    if (full_menu)
    {
        if ((g_cur_fractal_specific->tojulia != fractal_type::NOFRACTAL && g_params[0] == 0.0 && g_params[1] == 0.0)
            || g_cur_fractal_specific->tomandel != fractal_type::NOFRACTAL)
        {
            nextleft += 2;
            choicekey[nextleft] = FIK_SPACE;
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "toggle to/from julia <space>";
            showjuliatoggle = true;
        }
        if (g_fractal_type == fractal_type::JULIA || g_fractal_type == fractal_type::JULIAFP
            || g_fractal_type == fractal_type::INVERSEJULIA)
        {
            nextleft += 2;
            choicekey[nextleft] = 'j';
            attributes[nextleft] = MENU_ITEM;
            choices[nextleft] = "toggle to/from inverse <j>  ";
            showjuliatoggle = true;
        }

        nextleft += 2;
        choicekey[nextleft] = 'h';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "return to prior image  <h>   ";

        nextleft += 2;
        choicekey[nextleft] = FIK_BACKSPACE;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "reverse thru history <ctl-h> ";
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
    choices[nextleft] = "basic options...       <x>  ";

    nextleft += 2;
    choicekey[nextleft] = 'y';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "extended options...    <y>  ";

    nextleft += 2;
    choicekey[nextleft] = 'z';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "type-specific parms... <z>  ";

    nextleft += 2;
    choicekey[nextleft] = 'p';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "passes options...      <p>  ";

    nextleft += 2;
    choicekey[nextleft] = 'v';
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "view window options... <v>  ";

    if (!showjuliatoggle)
    {
        nextleft += 2;
        choicekey[nextleft] = 'i';
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "fractal 3D parms...    <i>  ";
    }

    nextleft += 2;
    choicekey[nextleft] = FIK_CTL_B;
    attributes[nextleft] = MENU_ITEM;
    choices[nextleft] = "browse parms...      <ctl-b>";

    if (full_menu)
    {
        nextleft += 2;
        choicekey[nextleft] = FIK_CTL_E;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "evolver parms...     <ctl-e>";

#ifndef XFRACT
        nextleft += 2;
        choicekey[nextleft] = FIK_CTL_F;
        attributes[nextleft] = MENU_ITEM;
        choices[nextleft] = "sound parms...       <ctl-f>";
#endif
    }

    nextright += 2;
    attributes[nextright] = 256 + MENU_HDG;
    choices[nextright] = "        FILE                  ";

    nextright += 2;
    choicekey[nextright] = '@';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "run saved command set... <@>  ";

    if (full_menu)
    {
        nextright += 2;
        choicekey[nextright] = 's';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "save image to file       <s>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'r';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "load image from file...  <r>  ";

    nextright += 2;
    choicekey[nextright] = '3';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "3d transform from file...<3>  ";

    if (full_menu)
    {
        nextright += 2;
        choicekey[nextright] = '#';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "3d overlay from file.....<#>  ";

        nextright += 2;
        choicekey[nextright] = 'b';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "save current parameters..<b>  ";

        nextright += 2;
        choicekey[nextright] = 16;
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "print image          <ctl-p>  ";
    }

    nextright += 2;
    choicekey[nextright] = 'd';
    attributes[nextright] = MENU_ITEM;
#ifdef XFRACT
    choices[nextright] = "shell to Linux/Unix      <d>  ";
#else
    choices[nextright] = "shell to dos             <d>  ";
#endif

    nextright += 2;
    choicekey[nextright] = 'g';
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "give command string      <g>  ";

    nextright += 2;
    choicekey[nextright] = FIK_ESC;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "quit id                  <esc> ";

    nextright += 2;
    choicekey[nextright] = FIK_INSERT;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "restart id               <ins> ";

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
        choices[nextright] = "color cycling mode       <c>  ";

        nextright += 2;
        choicekey[nextright] = '+';
        attributes[nextright] = MENU_ITEM;
        choices[nextright] = "rotate palette      <+>, <->  ";

        if (g_colors > 16)
        {
            nextright += 2;
            choicekey[nextright] = 'e';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "palette editing mode     <e>  ";

            nextright += 2;
            choicekey[nextright] = 'a';
            attributes[nextright] = MENU_ITEM;
            choices[nextright] = "make starfield           <a>  ";
        }
    }

    nextright += 2;
    choicekey[nextright] = FIK_CTL_A;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "ant automaton          <ctl-a>";

    nextright += 2;
    choicekey[nextright] = FIK_CTL_S;
    attributes[nextright] = MENU_ITEM;
    choices[nextright] = "stereogram             <ctl-s>";

    i = driver_key_pressed() ? driver_get_key() : 0;
    if (menu_checkkey(i, 0) == 0)
    {
        g_help_mode = help_labels::HELPMAIN;         // switch help modes
        nextleft += 2;
        if (nextleft < nextright)
        {
            nextleft = nextright + 1;
        }
        i = fullscreen_choice(CHOICE_MENU | CHOICE_CRUNCH,
                              "MAIN MENU",
                              nullptr, nullptr, nextleft, (char const **) choices, attributes,
                              2, nextleft/2, 29, 0, nullptr, nullptr, nullptr, menu_checkkey);
        if (i == -1)     // escape
        {
            i = FIK_ESC;
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
                g_help_mode = help_labels::HELPZOOM;
                help(0);
                i = 0;
            }
        }
    }
    if (i == FIK_ESC)             // escape from menu exits
    {
        helptitle();
        driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
        for (int j = 9; j <= 11; ++j)
        {
            driver_set_attr(j, 18, C_GENERAL_INPUT, 40);
        }
        putstringcenter(10, 18, 40, C_GENERAL_INPUT,
                        "Exit from Iterated Dynamics (y/n)? y"
                       );
        driver_hide_text_cursor();
        while ((i = driver_get_key()) != 'y' && i != 'Y' && i != FIK_ENTER)
        {
            if (i == 'n' || i == 'N')
            {
                goto top;
            }
        }
        goodbye();
    }
    if (i == FIK_TAB)
    {
        tab_display();
        i = 0;
    }
    if (i == FIK_ENTER || i == FIK_ENTER_2)
    {
        i = 0;                 // don't trigger new calc
    }
    g_tab_mode = old_tab_mode;
    return i;
}
