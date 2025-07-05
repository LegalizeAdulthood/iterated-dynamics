// SPDX-License-Identifier: GPL-3.0-only
//
//****************** standalone engine for "cellular" *******************

#include "fractals/cellular.h"

#include "engine/calcfrac.h"
#include "engine/engine_timer.h"
#include "engine/id_data.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "misc/Driver.h"
#include "ui/cmdfiles.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/stop_msg.h"
#include "ui/thinking.h"
#include "ui/video.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

//****************** standalone engine for "cellular" *******************

bool g_cellular_next_screen{};             // for cellular next screen generation

namespace id::fractals
{

enum
{
    BAD_T = 1,
    BAD_MEM = 2,
    STRING1 = 3,
    STRING2 = 4,
    TABLE_K = 5,
    TYPE_KR = 6,
    RULE_LENGTH = 7,
    INTERRUPT = 8,
};

static void set_cellular_palette();

inline char to_digit(int value)
{
    assert(value >= 0);
    assert(value <= 9);
    return static_cast<char>(value) + '0';
}

inline U16 from_digit(char value)
{
    assert(value >= '0');
    assert(value <= '9');
    return static_cast<U16>(value - '0');
}

Cellular::Cellular()
{
    set_cellular_palette();

    rand_param = (S32) g_params[0];
    kr = (U16) g_params[2];
    line_num = (U32) g_params[3];

    switch (kr)
    {
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 31:
    case 32:
    case 33:
    case 41:
    case 42:
    case 51:
    case 61:
        break;
    default:
        throw CellularError(*this, TYPE_KR);
    }

    m_s_r = (S16)(kr % 10); // Number of nearest neighbors to sum
    k = (U16) (kr / 10); // Number of different states, k=3 has states 0,1,2
    m_k_1 = (S16)(k - 1); // Highest state value, k=3 has highest state value of 2
    m_rule_digits = (S16)((m_s_r * 2 + 1) * m_k_1 + 1); // Number of digits in the rule

    if (!g_random_seed_flag && rand_param == -1)
    {
        --g_random_seed;
    }

    if (rand_param != 0 && rand_param != -1)
    {
        double n = g_params[0];
        char buf[512];
        std::snprintf(buf, std::size(buf), "%.16g", n); // # of digits in initial string
        S16 t = (S16)std::strlen(buf);
        if (t>16 || t <= 0)
        {
            throw CellularError(*this, STRING1);
        }
        for (U16 &elem : init_string)
        {
            elem = 0; // zero the array
        }
        S16 t2 = (S16) ((16 - t) / 2);
        for (int i = 0; i < t; i++)
        {
            // center initial string in array
            init_string[i+t2] = from_digit(buf[i]);
            if (init_string[i+t2]>(U16)m_k_1)
            {
                throw CellularError(*this, STRING2);
            }
        }
    }

    set_random_seed();

    // generate rule table from parameter 1
    double n = g_params[1];
    if (n == 0.0)
    {
        // calculate a random rule
        n = std::rand()%(int) k;
        for (int i = 1; i < m_rule_digits; i++)
        {
            n *= 10;
            n += std::rand()%(int) k;
        }
        g_params[1] = n;
    }
    char buf[512];
    std::snprintf(buf, std::size(buf), "%.*g", m_rule_digits, n);
    S16 t = (S16)std::strlen(buf);
    if (m_rule_digits < t || t < 0)
    {
        // leading 0s could make t smaller
        throw CellularError(*this, RULE_LENGTH);
    }
    for (int i = 0; i < m_rule_digits; i++)   // zero the table
    {
        cell_table[i] = 0;
    }
    for (int i = 0; i < t; i++)
    {
        // reverse order
        cell_table[i] = from_digit(buf[t - i - 1]);
        if (cell_table[i]>(U16)m_k_1)
        {
            throw CellularError(*this, TABLE_K);
        }
    }

    m_cell_array[0].resize(g_i_stop_pt.x+1);
    m_cell_array[1].resize(g_i_stop_pt.x+1);

    // g_cellular_next_screen toggled by space bar, true for continuous
    // false to stop on next screen

    if (g_resuming && !g_cellular_next_screen && !m_last_screen_flag)
    {
        start_resume();
        get_resume(start_row);
        end_resume();
        read_span(start_row, 0, g_i_stop_pt.x, m_cell_array[filled].data());
    }
    else if (g_cellular_next_screen && !m_last_screen_flag)
    {
        start_resume();
        end_resume();
        read_span(g_i_stop_pt.y, 0, g_i_stop_pt.x, m_cell_array[filled].data());
        g_params[3] += g_i_stop_pt.y + 1;
        start_row = -1; // after 1st iteration its = 0
    }
    else
    {
        if (g_random_seed_flag || rand_param == 0 || rand_param == -1)
        {
            for (g_col = 0; g_col <= g_i_stop_pt.x; g_col++)
            {
                m_cell_array[filled][g_col] = (Byte)(std::rand()%(int) k);
            }
        } // end of if random
        else
        {
            for (g_col = 0; g_col <= g_i_stop_pt.x; g_col++)
            {
                // Clear from end to end
                m_cell_array[filled][g_col] = 0;
            }
            int i = 0;
            for (g_col = (g_i_stop_pt.x-16)/2; g_col < (g_i_stop_pt.x+16)/2; g_col++)
            {
                // insert initial
                m_cell_array[filled][g_col] = (Byte)init_string[i++];    // string
            }
        } // end of if not random
        m_last_screen_flag = line_num != 0;
        write_span(start_row, 0, g_i_stop_pt.x, m_cell_array[filled].data());
    }
    start_row++;

    g_row = start_row;
}

// This section does all the work
bool Cellular::iterate()
{
    // This section calculates the starting line when it is not zero
    // This section can't be resumed since no screen output is generated
    // calculates the (line_num - 1) generation
    if (m_last_screen_flag)   // line number != 0 & not resuming & not continuing
    {
        if (g_row < static_cast<int>(line_num))
        {
            thinking("Cellular thinking (higher start row takes longer)");
            if (g_random_seed_flag || rand_param == 0 || rand_param == -1)
            {
                // Use a random border
                for (int i = 0; i <= m_s_r; i++)
                {
                    m_cell_array[not_filled][i] = (Byte)(std::rand()%(int) k);
                    m_cell_array[not_filled][g_i_stop_pt.x-i] = (Byte)(std::rand()%(int) k);
                }
            }
            else
            {
                // Use a zero border
                for (int i = 0; i <= m_s_r; i++)
                {
                    m_cell_array[not_filled][i] = 0;
                    m_cell_array[not_filled][g_i_stop_pt.x-i] = 0;
                }
            }

            S16 t = 0; // do first cell
            U16 two_r = (U16)(m_s_r+m_s_r);
            for (int i = 0; i <= two_r; i++)
            {
                t = (S16)(t + (S16)m_cell_array[filled][i]);
            }
            if (t > m_rule_digits || t < 0)
            {
                thinking_end();
                throw CellularError(*this, BAD_T, t);
            }
            m_cell_array[not_filled][m_s_r] = (Byte)cell_table[t];

            // use a rolling sum in t
            for (g_col = m_s_r+1; g_col < g_i_stop_pt.x-m_s_r; g_col++)
            {
                // now do the rest
                t = (S16)(t + m_cell_array[filled][g_col+m_s_r] - m_cell_array[filled][g_col-m_s_r-1]);
                if (t > m_rule_digits || t < 0)
                {
                    thinking_end();
                    throw CellularError(*this, BAD_T, t);
                }
                m_cell_array[not_filled][g_col] = (Byte)cell_table[t];
            }

            filled = not_filled;
            not_filled = (S16)(1-filled);
            ++g_row;
            return true;
        }
        start_row = 0;
        thinking_end();
        m_last_screen_flag = false;
    }

    if (g_row <= g_i_stop_pt.y)
    {
        if (g_random_seed_flag || rand_param == 0 || rand_param == -1)
        {
            // Use a random border
            for (int i = 0; i <= m_s_r; i++)
            {
                m_cell_array[not_filled][i] = (Byte)(std::rand()%(int) k);
                m_cell_array[not_filled][g_i_stop_pt.x-i] = (Byte)(std::rand()%(int) k);
            }
        }
        else
        {
            // Use a zero border
            for (int i = 0; i <= m_s_r; i++)
            {
                m_cell_array[not_filled][i] = 0;
                m_cell_array[not_filled][g_i_stop_pt.x-i] = 0;
            }
        }

        S16 t = 0; // do first cell
        U16 two_r = (U16)(m_s_r+m_s_r);
        for (int i = 0; i <= two_r; i++)
        {
            t = (S16)(t + (S16)m_cell_array[filled][i]);
        }
        if (t > m_rule_digits || t < 0)
        {
            thinking_end();
            throw CellularError(*this, BAD_T, t);
        }
        m_cell_array[not_filled][m_s_r] = (Byte)cell_table[t];

        // use a rolling sum in t
        for (g_col = m_s_r+1; g_col < g_i_stop_pt.x-m_s_r; g_col++)
        {
            // now do the rest
            t = (S16)(t + m_cell_array[filled][g_col+m_s_r] - m_cell_array[filled][g_col-m_s_r-1]);
            if (t > m_rule_digits || t < 0)
            {
                thinking_end();
                throw CellularError(*this, BAD_T, t);
            }
            m_cell_array[not_filled][g_col] = (Byte)cell_table[t];
        }

        filled = not_filled;
        not_filled = (S16)(1-filled);
        write_span(g_row, 0, g_i_stop_pt.x, m_cell_array[filled].data());
        ++g_row;
        return true;
    }

    if (g_cellular_next_screen)
    {
        g_params[3] += g_i_stop_pt.y + 1;
        start_row = 0;
        return true;
    }

    return false;
}

void Cellular::suspend()
{
    if (m_last_screen_flag)
    {
        thinking_end();
        throw CellularError(*this, INTERRUPT);
    }
    alloc_resume(10, 1);
    put_resume(g_row);
}

std::string Cellular::error(int err, int t) const
{
    switch (err)
    {
    case BAD_T:
    {
        char msg[30];
        std::snprintf(msg, std::size(msg), "Bad t=%d, aborting\n", t);
        return msg;
    }

    case BAD_MEM:
        return "Insufficient free memory for calculation";

    case STRING1:
        return "String can be a maximum of 16 digits";

    case STRING2:
    {
        static char msg[] = {"Make string of 0's through  's"};
        msg[27] = to_digit(m_k_1);
        return msg;
    }

    case TABLE_K:
    {
        static char msg[] = {"Make Rule with 0's through  's"};
        msg[27] = to_digit(m_k_1);
        return msg;
    }

    case TYPE_KR:
        return "Type must be 21, 31, 41, 51, 61, 22, 32, 42, 23, 33, 24, 25, 26, 27";

    case RULE_LENGTH:
    {
        static char msg[] = {"Rule must be    digits long"};
        int i = m_rule_digits / 10;
        if (i == 0)
        {
            msg[14] = to_digit(m_rule_digits);
        }
        else
        {
            msg[13] = to_digit(i);
            msg[14] = to_digit(m_rule_digits % 10);
        }
        return msg;
    }

    case INTERRUPT:
        return "Interrupted, can't resume";
    }

    return "Unknown error";
}

static void set_cellular_palette()
{
    static const Byte RED[3]    = { 170, 0, 0 };
    static const Byte GREEN[3]  = { 40, 141, 40 };
    static const Byte BLUE[3]   = { 52, 48, 117 };
    static const Byte YELLOW[3] = { 242, 234, 73 };
    static const Byte BROWN[3]  = { 170, 85, 0 };

    if (g_map_specified && g_color_state != ColorState::DEFAULT)
    {
        return;       // map= specified
    }

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;

    g_dac_box[1][0] = RED[0];
    g_dac_box[1][1] = RED[1];
    g_dac_box[1][2] = RED[2];
    g_dac_box[2][0] = GREEN[0];
    g_dac_box[2][1] = GREEN[1];
    g_dac_box[2][2] = GREEN[2];
    g_dac_box[3][0] = BLUE[0];
    g_dac_box[3][1] = BLUE[1];
    g_dac_box[3][2] = BLUE[2];
    g_dac_box[4][0] = YELLOW[0];
    g_dac_box[4][1] = YELLOW[1];
    g_dac_box[4][2] = YELLOW[2];
    g_dac_box[5][0] = BROWN[0];
    g_dac_box[5][1] = BROWN[1];
    g_dac_box[5][2] = BROWN[2];

    spin_dac(0, 1);
}

} // namespace id::fractals

bool cellular_per_image()
{
    if (!g_resuming)
    {
        g_cellular_next_screen = false; // initialize flag
    }
    engine_timer(g_cur_fractal_specific->calc_type);
    return false;
}
