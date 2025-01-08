// SPDX-License-Identifier: GPL-3.0-only
//
/*
    Routines that manipulate the Video DAC on VGA Adapters
*/
#include "rotate.h"

#include "ValueSaver.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "field_prompt.h"
#include "get_a_filename.h"
#include "helpdefs.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "loadmap.h"
#include "math/rand15.h"
#include "merge_path_names.h"
#include "save_file.h"
#include "spindac.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static void pause_rotate();
static void set_palette(Byte start[3], Byte finish[3]);
static void set_palette2(Byte start[3], Byte finish[3]);
static void set_palette3(Byte start[3], Byte middle[3], Byte finish[3]);

static bool s_paused{};         // rotate-is-paused flag
static Byte s_red[3]{63, 0, 0}; // for shifted-Fkeys
static Byte s_green[3]{0, 63, 0};
static Byte s_blue[3]{0, 0, 63};
static Byte s_black[3]{0, 0, 0};
static Byte s_white[3]{63, 63, 63};
static Byte s_yellow[3]{63, 63, 0};
static Byte s_brown[3]{31, 31, 0};

std::string g_map_name;
bool g_map_set{};
Byte g_dac_box[256][3]{};
Byte g_old_dac_box[256][3]{};
bool g_dac_learn{};
bool g_got_real_dac{}; // true if load_dac has a dacbox

void rotate(int direction)      // rotate-the-palette routine
{
    int key;
    int last;
    int next;
    int f_key;
    int step;
    int f_step;
    int j_step;
    int old_step;
    int incr;
    int from_red = 0;
    int from_blue = 0;
    int from_green = 0;
    int to_red = 0;
    int to_blue = 0;
    int to_green = 0;
    int change_color;
    int change_direction;
    int rotate_max;
    int rotate_size;

    static int f_steps[] = {2, 4, 8, 12, 16, 24, 32, 40, 54, 100}; // (for Fkeys)

    if (!g_got_real_dac                 // ??? no DAC to rotate!
        || g_colors < 16)
    {
        // strange things happen in 2x modes
        driver_buzzer(Buzzer::PROBLEM);
        return;
    }

    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_CYCLING};

    s_paused = false;                      // not paused
    f_key = 0;                            // no random coloring
    step = 1;
    old_step = step;                      // single-step
    f_step = 1;
    change_color = -1;                    // no color (rgb) to change
    change_direction = 0;                 // no color direction to change
    incr = 999;                          // ready to randomize
    std::srand((unsigned)std::time(nullptr));         // randomize things

    if (direction == 0)
    {
        // firing up in paused mode?
        pause_rotate();                    // then force a pause
        direction = 1;                    // and set a rotate direction
    }

    rotate_max = (g_color_cycle_range_hi < g_colors) ? g_color_cycle_range_hi : g_colors-1;
    rotate_size = rotate_max - g_color_cycle_range_lo + 1;
    last = rotate_max;                   // last box that was filled
    next = g_color_cycle_range_lo;                    // next box to be filled
    if (direction < 0)
    {
        last = g_color_cycle_range_lo;
        next = rotate_max;
    }

    bool more = true;
    while (more)
    {
        if (driver_is_disk())
        {
            if (!s_paused)
            {
                pause_rotate();
            }
        }
        else
        {
            while (!driver_key_pressed())
            {
                // rotate until key hit, at least once so step=oldstep ok
                if (f_key > 0)
                {
                    // randomizing is on
                    for (int i_step = 0; i_step < step; i_step++)
                    {
                        j_step = next + (i_step * direction);
                        while (j_step < g_color_cycle_range_lo)
                        {
                            j_step += rotate_size;
                        }
                        while (j_step > rotate_max)
                        {
                            j_step -= rotate_size;
                        }
                        if (++incr > f_step)
                        {
                            // time to randomize
                            incr = 1;
                            f_step = ((f_steps[f_key-1]* (rand15() >> 8)) >> 6) + 1;
                            from_red   = g_dac_box[last][0];
                            from_green = g_dac_box[last][1];
                            from_blue  = g_dac_box[last][2];
                            to_red     = rand15() >> 9;
                            to_green   = rand15() >> 9;
                            to_blue    = rand15() >> 9;
                        }
                        g_dac_box[j_step][0] = (Byte)(from_red   + (((to_red    - from_red)*incr)/f_step));
                        g_dac_box[j_step][1] = (Byte)(from_green + (((to_green - from_green)*incr)/f_step));
                        g_dac_box[j_step][2] = (Byte)(from_blue  + (((to_blue  - from_blue)*incr)/f_step));
                    }
                }
                if (step >= rotate_size)
                {
                    step = old_step;
                }
                spin_dac(direction, step);
            }
        }
        if (step >= rotate_size)
        {
            step = old_step;
        }
        key = driver_get_key();
        if (s_paused
            && (key != ' '
                && key != 'c'
                && key != ID_KEY_HOME
                && key != 'C'))
        {
            s_paused = false;                 // clear paused condition
        }
        switch (key)
        {
        case '+':                      // '+' means rotate forward
        case ID_KEY_RIGHT_ARROW:              // RightArrow = rotate fwd
            f_key = 0;
            direction = 1;
            last = rotate_max;
            next = g_color_cycle_range_lo;
            incr = 999;
            break;
        case '-':                      // '-' means rotate backward
        case ID_KEY_LEFT_ARROW:               // LeftArrow = rotate bkwd
            f_key = 0;
            direction = -1;
            last = g_color_cycle_range_lo;
            next = rotate_max;
            incr = 999;
            break;
        case ID_KEY_UP_ARROW:                 // UpArrow means speed up
            g_dac_learn = true;
            if (++g_dac_count >= g_colors)
            {
                --g_dac_count;
            }
            break;
        case ID_KEY_DOWN_ARROW:               // DownArrow means slow down
            g_dac_learn = true;
            if (g_dac_count > 1)
            {
                g_dac_count--;
            }
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            step = key - '0';   // change step-size
            step = std::min(step, rotate_size);
            break;
        case ID_KEY_F1:                       // ID_KEY_F1 - ID_KEY_F10:
        case ID_KEY_F2:                       // select a shading factor
        case ID_KEY_F3:
        case ID_KEY_F4:
        case ID_KEY_F5:
        case ID_KEY_F6:
        case ID_KEY_F7:
        case ID_KEY_F8:
        case ID_KEY_F9:
        case ID_KEY_F10:
            f_key = key - (ID_KEY_F1 - 1);
            f_step = 1;
            incr = 999;
            break;
        case ID_KEY_ENTER:                    // enter key: randomize all colors
        case ID_KEY_ENTER_2:                  // also the Numeric-Keypad Enter
            f_key = rand15()/3277 + 1;
            f_step = 1;
            incr = 999;
            old_step = step;
            step = rotate_size;
            break;
        case 'r':                      // color changes
            if (change_color    == -1)
            {
                change_color = 0;
            }
        case 'g':                      // color changes
            if (change_color    == -1)
            {
                change_color = 1;
            }
        case 'b':                      // color changes
            if (change_color    == -1)
            {
                change_color = 2;
            }
            if (change_direction == 0)
            {
                change_direction = -1;
            }
        case 'R':                      // color changes
            if (change_color    == -1)
            {
                change_color = 0;
            }
        case 'G':                      // color changes
            if (change_color    == -1)
            {
                change_color = 1;
            }
        case 'B':                      // color changes
            if (driver_is_disk())
            {
                break;
            }
            if (change_color    == -1)
            {
                change_color = 2;
            }
            if (change_direction == 0)
            {
                change_direction = 1;
            }
            for (int i = 1; i < 256; i++)
            {
                g_dac_box[i][change_color] = (Byte)(g_dac_box[i][change_color] + change_direction);
                if (g_dac_box[i][change_color] == 64)
                {
                    g_dac_box[i][change_color] = 63;
                }
                if (g_dac_box[i][change_color] == 255)
                {
                    g_dac_box[i][change_color] = 0;
                }
            }
            change_color    = -1;        // clear flags for next time
            change_direction = 0;
            s_paused          = false;    // clear any pause
        case ' ':                      // use the spacebar as a "pause" toggle
        case 'c':                      // for completeness' sake, the 'c' too
        case 'C':
            pause_rotate();              // pause
            break;
        case '>':                      // single-step
        case '.':
        case '<':
        case ',':
            if (key == '>' || key == '.')
            {
                direction = -1;
                last = g_color_cycle_range_lo;
                next = rotate_max;
                incr = 999;
            }
            else
            {
                direction = 1;
                last = rotate_max;
                next = g_color_cycle_range_lo;
                incr = 999;
            }
            f_key = 0;
            spin_dac(direction, 1);
            if (! s_paused)
            {
                pause_rotate();           // pause
            }
            break;
        case 'd':                      // load colors from "default.map"
        case 'D':
            if (validate_luts("default"))
            {
                break;
            }
            f_key = 0;                   // disable random generation
            pause_rotate();              // update palette and pause
            break;
        case 'a':                      // load colors from "altern.map"
        case 'A':
            if (validate_luts("altern"))
            {
                break;
            }
            f_key = 0;                   // disable random generation
            pause_rotate();              // update palette and pause
            break;
        case 'l':                      // load colors from a specified map
        case 'L':
            load_palette();
            f_key = 0;                   // disable random generation
            pause_rotate();              // update palette and pause
            break;
        case 's':                      // save the palette
        case 'S':
            save_palette();
            f_key = 0;                   // disable random generation
            pause_rotate();              // update palette and pause
            break;
        case ID_KEY_ESC:                      // escape
            more = false;                   // time to bail out
            break;
        case ID_KEY_HOME:                     // restore palette
            std::memcpy(g_dac_box, g_old_dac_box, 256*3);
            pause_rotate();              // pause
            break;
        default:                       // maybe a new palette
            f_key = 0;                   // disable random generation
            if (key == ID_KEY_SHF_F1)
            {
                set_palette(s_black, s_white);
            }
            if (key == ID_KEY_SHF_F2)
            {
                set_palette(s_red, s_yellow);
            }
            if (key == ID_KEY_SHF_F3)
            {
                set_palette(s_blue, s_green);
            }
            if (key == ID_KEY_SHF_F4)
            {
                set_palette(s_black, s_yellow);
            }
            if (key == ID_KEY_SHF_F5)
            {
                set_palette(s_black, s_red);
            }
            if (key == ID_KEY_SHF_F6)
            {
                set_palette(s_black, s_blue);
            }
            if (key == ID_KEY_SHF_F7)
            {
                set_palette(s_black, s_green);
            }
            if (key == ID_KEY_SHF_F8)
            {
                set_palette(s_blue, s_yellow);
            }
            if (key == ID_KEY_SHF_F9)
            {
                set_palette(s_red, s_green);
            }
            if (key == ID_KEY_SHF_F10)
            {
                set_palette(s_green, s_white);
            }
            if (key == ID_KEY_CTL_F1)
            {
                set_palette2(s_black, s_white);
            }
            if (key == ID_KEY_CTL_F2)
            {
                set_palette2(s_red, s_yellow);
            }
            if (key == ID_KEY_CTL_F3)
            {
                set_palette2(s_blue, s_green);
            }
            if (key == ID_KEY_CTL_F4)
            {
                set_palette2(s_black, s_yellow);
            }
            if (key == ID_KEY_CTL_F5)
            {
                set_palette2(s_black, s_red);
            }
            if (key == ID_KEY_CTL_F6)
            {
                set_palette2(s_black, s_blue);
            }
            if (key == ID_KEY_CTL_F7)
            {
                set_palette2(s_black, s_green);
            }
            if (key == ID_KEY_CTL_F8)
            {
                set_palette2(s_blue, s_yellow);
            }
            if (key == ID_KEY_CTL_F9)
            {
                set_palette2(s_red, s_green);
            }
            if (key == ID_KEY_CTL_F10)
            {
                set_palette2(s_green, s_white);
            }
            if (key == ID_KEY_ALT_F1)
            {
                set_palette3(s_blue, s_green, s_red);
            }
            if (key == ID_KEY_ALT_F2)
            {
                set_palette3(s_blue, s_yellow, s_red);
            }
            if (key == ID_KEY_ALT_F3)
            {
                set_palette3(s_red, s_white, s_blue);
            }
            if (key == ID_KEY_ALT_F4)
            {
                set_palette3(s_red, s_yellow, s_white);
            }
            if (key == ID_KEY_ALT_F5)
            {
                set_palette3(s_black, s_brown, s_yellow);
            }
            if (key == ID_KEY_ALT_F6)
            {
                set_palette3(s_blue, s_brown, s_green);
            }
            if (key == ID_KEY_ALT_F7)
            {
                set_palette3(s_blue, s_green, s_green);
            }
            if (key == ID_KEY_ALT_F8)
            {
                set_palette3(s_blue, s_green, s_white);
            }
            if (key == ID_KEY_ALT_F9)
            {
                set_palette3(s_green, s_green, s_white);
            }
            if (key == ID_KEY_ALT_F10)
            {
                set_palette3(s_red, s_blue, s_white);
            }
            pause_rotate();  // update palette and pause
            break;
        }
    }
}

static void pause_rotate()               // pause-the-rotate routine
{
    if (s_paused)                            // if already paused , just clear
    {
        s_paused = false;
    }
    else
    {
        // else set border, wait for a key
        int old_dac_count = g_dac_count;  // saved dac-count value goes here
        Byte old_dac0 = g_dac_box[0][0];
        Byte old_dac1 = g_dac_box[0][1];
        Byte old_dac2 = g_dac_box[0][2];
        g_dac_count = 256;
        g_dac_box[0][0] = 48;
        g_dac_box[0][1] = 48;
        g_dac_box[0][2] = 48;
        spin_dac(0, 1);                     // show white border
        if (driver_is_disk())
        {
            dvid_status(100, " Paused in \"color cycling\" mode ");
        }
        driver_wait_key_pressed(0);                // wait for any key

        if (driver_is_disk())
        {
            dvid_status(0, "");
        }
        g_dac_box[0][0] = old_dac0;
        g_dac_box[0][1] = old_dac1;
        g_dac_box[0][2] = old_dac2;
        spin_dac(0, 1);                     // show black border
        g_dac_count = old_dac_count;
        s_paused = true;
    }
}

static void set_palette(Byte start[3], Byte finish[3])
{
    g_dac_box[0][2] = 0;
    g_dac_box[0][1] = g_dac_box[0][2];
    g_dac_box[0][0] = g_dac_box[0][1];
    for (int i = 1; i <= 255; i++)                    // fill the palette
    {
        for (int j = 0; j < 3; j++)
        {
            g_dac_box[i][j] = (Byte)((i*start[j] + (256-i)*finish[j])/255);
        }
    }
}

static void set_palette2(Byte start[3], Byte finish[3])
{
    g_dac_box[0][2] = 0;
    g_dac_box[0][1] = g_dac_box[0][2];
    g_dac_box[0][0] = g_dac_box[0][1];
    for (int i = 1; i <= 128; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            g_dac_box[i][j]     = (Byte)((i*finish[j] + (128-i)*start[j])/128);
            g_dac_box[i+127][j] = (Byte)((i*start[j]  + (128-i)*finish[j])/128);
        }
    }
}

static void set_palette3(Byte start[3], Byte middle[3], Byte finish[3])
{
    g_dac_box[0][2] = 0;
    g_dac_box[0][1] = g_dac_box[0][2];
    g_dac_box[0][0] = g_dac_box[0][1];
    for (int i = 1; i <= 85; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            g_dac_box[i][j]     = (Byte)((i*middle[j] + (86-i)*start[j])/85);
            g_dac_box[i+85][j]  = (Byte)((i*finish[j] + (86-i)*middle[j])/85);
            g_dac_box[i+170][j] = (Byte)((i*start[j]  + (86-i)*finish[j])/85);
        }
    }
}

void save_palette()
{
    char pal_name[FILE_MAX_PATH];
    std::strcpy(pal_name, g_map_name.c_str());
    driver_stack_screen();
    char filename[256]{};
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_COLORMAP};
    int i = field_prompt("Name of map file to write", nullptr, filename, 60, nullptr);
    driver_unstack_screen();
    if (i != -1 && filename[0])
    {
        if (std::strchr(filename, '.') == nullptr)
        {
            std::strcat(filename, ".map");
        }
        merge_path_names(pal_name, filename, CmdFile::AT_AFTER_STARTUP);
        std::FILE *dac_file = open_save_file(pal_name, "w");
        if (dac_file == nullptr)
        {
            driver_buzzer(Buzzer::PROBLEM);
        }
        else
        {
            for (i = 0; i < g_colors; i++)
            {
                std::fprintf(dac_file, "%3d %3d %3d\n",
                        g_dac_box[i][0] << 2,
                        g_dac_box[i][1] << 2,
                        g_dac_box[i][2] << 2);
            }
            std::memcpy(g_old_dac_box, g_dac_box, 256*3);
            g_color_state = ColorState::MAP_FILE;
            g_color_file = filename;
        }
        std::fclose(dac_file);
    }
}

bool load_palette()
{
    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_COLORMAP};
    std::string filename{g_map_name};
    driver_stack_screen();
    const bool i = get_a_file_name("Select a MAP File", "*.map", filename);
    driver_unstack_screen();
    if (!i)
    {
        if (!validate_luts(filename.c_str()))
        {
            std::memcpy(g_old_dac_box, g_dac_box, 256*3);
        }
        merge_path_names(g_map_name, filename.c_str(), CmdFile::AT_CMD_LINE);
    }
    return i;
}
