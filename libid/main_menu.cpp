// SPDX-License-Identifier: GPL-3.0-only
//
#include "main_menu.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "goodbye.h"
#include "help_title.h"
#include "helpcom.h"
#include "id_data.h"
#include "id_keys.h"
#include "put_string_center.h"
#include "rotate.h"
#include "tab_display.h"
#include "ValueSaver.h"
#include "video_mode.h"

#include <cstring>

enum
{
    MENU_HDG = 3,
    MENU_ITEM = 1
};

static bool s_full_menu{};

static bool has_julia_toggle()
{
    return (g_cur_fractal_specific->tojulia != FractalType::NOFRACTAL && //
               g_params[0] == 0.0 &&                                      //
               g_params[1] == 0.0) ||
        g_cur_fractal_specific->tomandel != FractalType::NOFRACTAL;
}

static bool is_julia()
{
    return g_fractal_type == FractalType::JULIA   //
        || g_fractal_type == FractalType::JULIAFP //
        || g_fractal_type == FractalType::INVERSEJULIA;
}

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
            if (has_julia_toggle())
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

namespace
{

struct MainMenu
{
    MainMenu(bool full_menu);

    int prompt();

private:
    char const *m_choices[44]; // 2 columns * 22 rows
    int m_attributes[44];
    int m_choice_key[44];
    int m_next_left{};
    int m_next_right{};
};

MainMenu::MainMenu(bool full_menu)
{
    const auto add_left_heading{[&](const char *heading)
        {
            m_next_left += 2;
            m_choices[m_next_left] = heading;
            m_attributes[m_next_left] = 256 + MENU_HDG;
        }};
    const auto add_left_item{[&](const char *choice, int key)
        {
            m_next_left += 2;
            m_choice_key[m_next_left] = key;
            m_attributes[m_next_left] = MENU_ITEM;
            m_choices[m_next_left] = choice;
        }};
    const auto add_right_heading{[&](const char *heading)
        {
            m_next_right += 2;
            m_choices[m_next_right] = heading;
            m_attributes[m_next_right] = 256 + MENU_HDG;
        }};
    const auto add_right_item{[&](const char *choice, int key)
        {
            m_next_right += 2;
            m_choice_key[m_next_right] = key;
            m_attributes[m_next_right] = MENU_ITEM;
            m_choices[m_next_right] = choice;
        }};

    bool show_julia_toggle{};
    for (int j = 0; j < 44; ++j)
    {
        m_attributes[j] = 256;
        m_choices[j] = "";
        m_choice_key[j] = -1;
    }
    m_next_left = -2;
    m_next_right = -1;

    if (full_menu)
    {
        add_left_heading("      CURRENT IMAGE         ");
        add_left_item((g_calc_status == CalcStatus::RESUMABLE) ? "Continue calculation        "
                                                                      : "Return to image             ",
            ID_KEY_ENTER);
        add_left_item("Info about image      <Tab> ", ID_KEY_TAB);
        add_left_item("Orbits window          <O>  ", 'o');
    }
    add_left_heading("      NEW IMAGE             ");
    add_left_item("Select video mode...  <Del> ", ID_KEY_DELETE);
    add_left_item("Select fractal type    <T>  ", 't');
    if (full_menu)
    {
        if (has_julia_toggle())
        {
            add_left_item("Toggle to/from Julia <Space>", ID_KEY_SPACE);
            show_julia_toggle = true;
        }
        if (is_julia())
        {
            add_left_item("Toggle to/from inverse <J>  ", 'j');
            show_julia_toggle = true;
        }
        add_left_item("Return to prior image  <H>   ", 'h');
        add_left_item("Reverse thru history <Ctrl+H>", ID_KEY_BACKSPACE);
    }
    else
    {
        m_next_left += 2;
    }
    add_left_heading("      OPTIONS                ");
    add_left_item("Basic options...       <X>  ", 'x');
    add_left_item("Extended options...    <Y>  ", 'y');
    add_left_item("Type-specific params...<Z>  ", 'z');
    add_left_item("Passes options...      <P>  ", 'p');
    add_left_item("View window options... <V>  ", 'v');
    if (!show_julia_toggle)
    {
        add_left_item("Fractal 3D params...   <I>  ", 'i');
    }
    add_left_item("Browse params...    <Ctrl+B>", ID_KEY_CTL_B);
    if (full_menu)
    {
        add_left_item("Evolver params...   <Ctrl+E>", ID_KEY_CTL_E);
        add_left_item("Sound params...     <Ctrl+F>", ID_KEY_CTL_F);
    }

    add_right_heading("        FILE                  ");
    add_right_item("Run saved param set...   <@>  ", '@');
    if (full_menu)
    {
        add_right_item("Save image to file       <S>  ", 's');
    }
    add_right_item("Load image from file...  <R>  ", 'r');
    add_right_item("3D transform from file...<3>  ", '3');
    if (full_menu)
    {
        add_right_item("3D overlay from file...  <#>  ", '#');
        add_right_item("Save current parameters..<B>  ", 'b');
    }
    add_right_item("Command shell            <D>  ", 'd');
    add_right_item("Give parameter string    <G>  ", 'g');
    add_right_item("Quit Id                  <Esc> ", ID_KEY_ESC);
    add_right_item("Restart Id               <Ins> ", ID_KEY_INSERT);
    if (full_menu && g_got_real_dac && g_colors >= 16)
    {
        add_right_heading("       COLORS                 ");
        add_right_item("Color cycling mode       <C>  ",'c');
        add_right_item("Rotate palette      <+>, <->  ", '+');
        if (g_colors > 16)
        {
            add_right_item("Palette editing mode     <E>  ", 'e');
            add_right_item("Make starfield           <A>  ", 'a');
        }
    }
    add_right_item("Ant automaton         <Ctrl+A>", ID_KEY_CTL_A);
    add_right_item("Stereogram            <Ctrl+S>", ID_KEY_CTL_S);
}

int MainMenu::prompt()
{
    int i = driver_key_pressed() ? driver_get_key() : 0;
    if (menu_check_key(i, 0) == 0)
    {
        g_help_mode = HelpLabels::HELP_MAIN;         // switch help modes
        m_next_left += 2;
        if (m_next_left < m_next_right)
        {
            m_next_left = m_next_right + 1;
        }
        i = full_screen_choice(ChoiceFlags::MENU | ChoiceFlags::CRUNCH, "MAIN MENU", nullptr, nullptr, m_next_left,
            m_choices, m_attributes, 2, m_next_left / 2, 29, 0, nullptr, nullptr, nullptr, menu_check_key);
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
            i = m_choice_key[i];
            if (i == -ID_KEY_CTL_ENTER)
            {
                g_help_mode = HelpLabels::HELP_ZOOM;
                help();
                i = 0;
            }
        }
    }
    return i;
}

} // namespace

static bool exit_prompt()
{
    help_title();
    driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
    for (int j = 9; j <= 11; ++j)
    {
        driver_set_attr(j, 18, C_GENERAL_INPUT, 40);
    }
    put_string_center(10, 18, 40, C_GENERAL_INPUT, "Exit from " ID_PROGRAM_NAME " (y/n)? y");
    driver_hide_text_cursor();
    int i;
    while ((i = driver_get_key()) != 'y' && i != 'Y' && i != ID_KEY_ENTER)
    {
        if (i == 'n' || i == 'N')
        {
            return false;
        }
    }
    return true;
}

int main_menu(bool full_menu)
{
    ValueSaver saved_tab_mode{g_tab_mode};

    bool prompt{true};
    int key{};
    while (prompt)
    {
        s_full_menu = full_menu;
        g_tab_mode = false;
        MainMenu menu(full_menu);

        key = menu.prompt();
        if (key == ID_KEY_ESC)             // escape from menu exits
        {
            if (exit_prompt())
            {
                goodbye();
            }
        }
        else
        {
            prompt = false;
        }
    }
    if (key == ID_KEY_TAB)
    {
        tab_display();
        key = 0;
    }
    if (key == ID_KEY_ENTER || key == ID_KEY_ENTER_2)
    {
        key = 0;                 // don't trigger new calc
    }
    return key;
}
