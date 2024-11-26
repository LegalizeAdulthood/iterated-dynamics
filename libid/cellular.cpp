// SPDX-License-Identifier: GPL-3.0-only
//
//****************** standalone engine for "cellular" *******************

#include "cellular.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "engine_timer.h"
#include "fractalp.h"
#include "id_data.h"
#include "resume.h"
#include "rotate.h"
#include "spindac.h"
#include "stop_msg.h"
#include "thinking.h"
#include "video.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

//****************** standalone engine for "cellular" *******************

enum
{
    BAD_T = 1,
    BAD_MEM = 2,
    STRING1 = 3,
    STRING2 = 4,
    TABLEK = 5,
    TYPEKR = 6,
    RULELENGTH = 7,
    INTERUPT = 8
};

enum
{
    CELLULAR_DONE = 10
};

static void set_Cellular_palette();

static std::vector<BYTE> s_cell_array[2];
static S16 s_s_r{};
static S16 s_k_1{};
static S16 s_rule_digits{};
static bool s_last_screen_flag{};

bool g_cellular_next_screen{};             // for cellular next screen generation

void abort_cellular(int err, int t)
{
    int i;
    switch (err)
    {
    case BAD_T:
    {
        char msg[30];
        std::snprintf(msg, std::size(msg), "Bad t=%d, aborting\n", t);
        stop_msg(msg);
    }
    break;
    case BAD_MEM:
    {
        stop_msg("Insufficient free memory for calculation");
    }
    break;
    case STRING1:
    {
        stop_msg("String can be a maximum of 16 digits");
    }
    break;
    case STRING2:
    {
        static char msg[] = {"Make string of 0's through  's" };
        msg[27] = (char)(s_k_1 + 48); // turn into a character value
        stop_msg(msg);
    }
    break;
    case TABLEK:
    {
        static char msg[] = {"Make Rule with 0's through  's" };
        msg[27] = (char)(s_k_1 + 48); // turn into a character value
        stop_msg(msg);
    }
    break;
    case TYPEKR:
    {
        stop_msg("Type must be 21, 31, 41, 51, 61, 22, 32, 42, 23, 33, 24, 25, 26, 27");
    }
    break;
    case RULELENGTH:
    {
        static char msg[] = {"Rule must be    digits long" };
        i = s_rule_digits / 10;
        if (i == 0)
        {
            msg[14] = (char)(s_rule_digits + 48);
        }
        else
        {
            msg[13] = (char)(i+48);
            msg[14] = (char)((s_rule_digits % 10) + 48);
        }
        stop_msg(msg);
    }
    break;
    case INTERUPT:
    {
        stop_msg("Interrupted, can't resume");
    }
    break;
    case CELLULAR_DONE:
        break;
    }
}

int cellular()
{
    S16 start_row;
    S16 filled;
    S16 notfilled;
    U16 cell_table[32];
    U16 init_string[16];
    U16 kr;
    U16 k;
    U32 lnnmbr;
    U16 twor;
    S16 t;
    S16 t2;
    S32 randparam;
    double n;
    char buf[512];

    set_Cellular_palette();

    randparam = (S32)g_params[0];
    lnnmbr = (U32)g_params[3];
    kr = (U16)g_params[2];
    switch (kr)
    {
    case 21:
    case 31:
    case 41:
    case 51:
    case 61:
    case 22:
    case 32:
    case 42:
    case 23:
    case 33:
    case 24:
    case 25:
    case 26:
    case 27:
        break;
    default:
        abort_cellular(TYPEKR, 0);
        return -1;
    }

    s_s_r = (S16)(kr % 10); // Number of nearest neighbors to sum
    k = (U16)(kr / 10); // Number of different states, k=3 has states 0,1,2
    s_k_1 = (S16)(k - 1); // Highest state value, k=3 has highest state value of 2
    s_rule_digits = (S16)((s_s_r * 2 + 1) * s_k_1 + 1); // Number of digits in the rule

    if (!g_random_seed_flag && randparam == -1)
    {
        --g_random_seed;
    }
    if (randparam != 0 && randparam != -1)
    {
        n = g_params[0];
        std::snprintf(buf, std::size(buf), "%.16g", n); // # of digits in initial string
        t = (S16)std::strlen(buf);
        if (t>16 || t <= 0)
        {
            abort_cellular(STRING1, 0);
            return -1;
        }
        for (U16 &elem : init_string)
        {
            elem = 0; // zero the array
        }
        t2 = (S16)((16 - t)/2);
        for (int i = 0; i < t; i++)
        {
            // center initial string in array
            init_string[i+t2] = (U16)(buf[i] - 48); // change character to number
            if (init_string[i+t2]>(U16)s_k_1)
            {
                abort_cellular(STRING2, 0);
                return -1;
            }
        }
    }

    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    // generate rule table from parameter 1
    n = g_params[1];
    if (n == 0)
    {
        // calculate a random rule
        n = std::rand()%(int)k;
        for (int i = 1; i < s_rule_digits; i++)
        {
            n *= 10;
            n += std::rand()%(int)k;
        }
        g_params[1] = n;
    }
    std::snprintf(buf, std::size(buf), "%.*g", s_rule_digits , n);
    t = (S16)std::strlen(buf);
    if (s_rule_digits < t || t < 0)
    {
        // leading 0s could make t smaller
        abort_cellular(RULELENGTH, 0);
        return -1;
    }
    for (int i = 0; i < s_rule_digits; i++)   // zero the table
    {
        cell_table[i] = 0;
    }
    for (int i = 0; i < t; i++)
    {
        // reverse order
        cell_table[i] = (U16)(buf[t-i-1] - 48); // change character to number
        if (cell_table[i]>(U16)s_k_1)
        {
            abort_cellular(TABLEK, 0);
            return -1;
        }
    }

    start_row = 0;
    bool resized = false;
    try
    {
        s_cell_array[0].resize(g_i_x_stop+1);
        s_cell_array[1].resize(g_i_x_stop+1);
        resized = true;
    }
    catch (std::bad_alloc const&)
    {
    }
    if (!resized)
    {
        abort_cellular(BAD_MEM, 0);
        return -1;
    }

    // nxtscreenflag toggled by space bar, true for continuous
    // false to stop on next screen

    filled = 0;
    notfilled = (S16)(1-filled);
    if (g_resuming && !g_cellular_next_screen && !s_last_screen_flag)
    {
        start_resume();
        get_resume(sizeof(start_row), &start_row, 0);
        end_resume();
        read_span(start_row, 0, g_i_x_stop, &s_cell_array[filled][0]);
    }
    else if (g_cellular_next_screen && !s_last_screen_flag)
    {
        start_resume();
        end_resume();
        read_span(g_i_y_stop, 0, g_i_x_stop, &s_cell_array[filled][0]);
        g_params[3] += g_i_y_stop + 1;
        start_row = -1; // after 1st iteration its = 0
    }
    else
    {
        if (g_random_seed_flag || randparam == 0 || randparam == -1)
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)
            {
                s_cell_array[filled][g_col] = (BYTE)(std::rand()%(int)k);
            }
        } // end of if random

        else
        {
            for (g_col = 0; g_col <= g_i_x_stop; g_col++)
            {
                // Clear from end to end
                s_cell_array[filled][g_col] = 0;
            }
            int i = 0;
            for (g_col = (g_i_x_stop-16)/2; g_col < (g_i_x_stop+16)/2; g_col++)
            {
                // insert initial
                s_cell_array[filled][g_col] = (BYTE)init_string[i++];    // string
            }
        } // end of if not random
        s_last_screen_flag = lnnmbr != 0;
        write_span(start_row, 0, g_i_x_stop, &s_cell_array[filled][0]);
    }
    start_row++;

    // This section calculates the starting line when it is not zero
    // This section can't be resumed since no screen output is generated
    // calculates the (lnnmbr - 1) generation
    if (s_last_screen_flag)   // line number != 0 & not resuming & not continuing
    {
        for (U32 big_row = (U32)start_row; big_row < lnnmbr; big_row++)
        {
            thinking(1, "Cellular thinking (higher start row takes longer)");
            if (g_random_seed_flag || randparam == 0 || randparam == -1)
            {
                // Use a random border
                for (int i = 0; i <= s_s_r; i++)
                {
                    s_cell_array[notfilled][i] = (BYTE)(std::rand()%(int)k);
                    s_cell_array[notfilled][g_i_x_stop-i] = (BYTE)(std::rand()%(int)k);
                }
            }
            else
            {
                // Use a zero border
                for (int i = 0; i <= s_s_r; i++)
                {
                    s_cell_array[notfilled][i] = 0;
                    s_cell_array[notfilled][g_i_x_stop-i] = 0;
                }
            }

            t = 0; // do first cell
            twor = (U16)(s_s_r+s_s_r);
            for (int i = 0; i <= twor; i++)
            {
                t = (S16)(t + (S16)s_cell_array[filled][i]);
            }
            if (t > s_rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return -1;
            }
            s_cell_array[notfilled][s_s_r] = (BYTE)cell_table[t];

            // use a rolling sum in t
            for (g_col = s_s_r+1; g_col < g_i_x_stop-s_s_r; g_col++)
            {
                // now do the rest
                t = (S16)(t + s_cell_array[filled][g_col+s_s_r] - s_cell_array[filled][g_col-s_s_r-1]);
                if (t > s_rule_digits || t < 0)
                {
                    thinking(0, nullptr);
                    abort_cellular(BAD_T, t);
                    return -1;
                }
                s_cell_array[notfilled][g_col] = (BYTE)cell_table[t];
            }

            filled = notfilled;
            notfilled = (S16)(1-filled);
            if (driver_key_pressed())
            {
                thinking(0, nullptr);
                abort_cellular(INTERUPT, 0);
                return -1;
            }
        }
        start_row = 0;
        thinking(0, nullptr);
        s_last_screen_flag = false;
    }

    // This section does all the work
contloop:
    for (g_row = start_row; g_row <= g_i_y_stop; g_row++)
    {
        if (g_random_seed_flag || randparam == 0 || randparam == -1)
        {
            // Use a random border
            for (int i = 0; i <= s_s_r; i++)
            {
                s_cell_array[notfilled][i] = (BYTE)(std::rand()%(int)k);
                s_cell_array[notfilled][g_i_x_stop-i] = (BYTE)(std::rand()%(int)k);
            }
        }
        else
        {
            // Use a zero border
            for (int i = 0; i <= s_s_r; i++)
            {
                s_cell_array[notfilled][i] = 0;
                s_cell_array[notfilled][g_i_x_stop-i] = 0;
            }
        }

        t = 0; // do first cell
        twor = (U16)(s_s_r+s_s_r);
        for (int i = 0; i <= twor; i++)
        {
            t = (S16)(t + (S16)s_cell_array[filled][i]);
        }
        if (t > s_rule_digits || t < 0)
        {
            thinking(0, nullptr);
            abort_cellular(BAD_T, t);
            return -1;
        }
        s_cell_array[notfilled][s_s_r] = (BYTE)cell_table[t];

        // use a rolling sum in t
        for (g_col = s_s_r+1; g_col < g_i_x_stop-s_s_r; g_col++)
        {
            // now do the rest
            t = (S16)(t + s_cell_array[filled][g_col+s_s_r] - s_cell_array[filled][g_col-s_s_r-1]);
            if (t > s_rule_digits || t < 0)
            {
                thinking(0, nullptr);
                abort_cellular(BAD_T, t);
                return -1;
            }
            s_cell_array[notfilled][g_col] = (BYTE)cell_table[t];
        }

        filled = notfilled;
        notfilled = (S16)(1-filled);
        write_span(g_row, 0, g_i_x_stop, &s_cell_array[filled][0]);
        if (driver_key_pressed())
        {
            abort_cellular(CELLULAR_DONE, 0);
            alloc_resume(10, 1);
            put_resume(sizeof(g_row), &g_row, 0);
            return -1;
        }
    }
    if (g_cellular_next_screen)
    {
        g_params[3] += g_i_y_stop + 1;
        start_row = 0;
        goto contloop;
    }
    abort_cellular(CELLULAR_DONE, 0);
    return 1;
}

bool cellular_setup()
{
    if (!g_resuming)
    {
        g_cellular_next_screen = false; // initialize flag
    }
    timer(timer_type::ENGINE, g_cur_fractal_specific->calctype);
    return false;
}

static void set_Cellular_palette()
{
    static BYTE const Red[3]    = { 42, 0, 0 };
    static BYTE const Green[3]  = { 10, 35, 10 };
    static BYTE const Blue[3]   = { 13, 12, 29 };
    static BYTE const Yellow[3] = { 60, 58, 18 };
    static BYTE const Brown[3]  = { 42, 21, 0 };

    if (g_map_specified && g_color_state != color_state::DEFAULT)
    {
        return;       // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;

    g_dac_box[1][0] = Red[0];
    g_dac_box[1][1] = Red[1];
    g_dac_box[1][2] = Red[2];
    g_dac_box[2][0] = Green[0];
    g_dac_box[2][1] = Green[1];
    g_dac_box[2][2] = Green[2];
    g_dac_box[3][0] = Blue[0];
    g_dac_box[3][1] = Blue[1];
    g_dac_box[3][2] = Blue[2];
    g_dac_box[4][0] = Yellow[0];
    g_dac_box[4][1] = Yellow[1];
    g_dac_box[4][2] = Yellow[2];
    g_dac_box[5][0] = Brown[0];
    g_dac_box[5][1] = Brown[1];
    g_dac_box[5][2] = Brown[2];

    spin_dac(0, 1);
}
