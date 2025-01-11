// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/log_map.h"

#include "engine/id_data.h"
#include "math/mpmath.h"
#include "ui/cmdfiles.h"

#include <cmath>

long g_log_map_flag{};                       // Logarithmic palette flag: 0 = no
int g_log_map_fly_calculate{};               // calculate logmap on-the-fly
bool g_log_map_auto_calculate{};             // auto calculate logmap
std::vector<Byte> g_log_map_table;
long g_log_map_table_max_size{};
bool g_log_map_calculate{};

static double s_mlf{};
static unsigned long s_lf{};

/* int LogFlag;
   LogFlag == 1  -- standard log palettes
   LogFlag == -1 -- 'old' log palettes
   LogFlag >  1  -- compress counts < LogFlag into color #1
   LogFlag < -1  -- use quadratic palettes based on square roots && compress
*/
void setup_log_table()
{
    float l, f, c, m;
    unsigned long limit;

    // set up on-the-fly variables
    if (g_log_map_flag > 0)
    {
        // new log function
        s_lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        s_mlf = (g_colors - (s_lf?2:1)) / std::log(static_cast<double>(g_log_map_table_max_size - s_lf));
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        s_mlf = (g_colors - 1) / std::log(static_cast<double>(g_log_map_table_max_size));
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        s_lf = 0 - g_log_map_flag;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        s_mlf = (g_colors - 2) / std::sqrt(static_cast<double>(g_log_map_table_max_size - s_lf));
    }

    if (g_log_map_calculate)
    {
        return; // LogTable not defined, bail out now
    }

    if (!g_log_map_calculate)
    {
        g_log_map_calculate = true;   // turn it on
        for (unsigned long prev = 0U; prev <= (unsigned long)g_log_map_table_max_size; prev++)
        {
            g_log_map_table[prev] = (Byte)log_table_calc((long)prev);
        }
        g_log_map_calculate = false;   // turn it off, again
        return;
    }

    if (g_log_map_flag > -2)
    {
        s_lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        fg_to_float((long)(g_log_map_table_max_size-s_lf), 0, m);
        f_log14(m, m);
        fg_to_float((long)(g_colors-(s_lf?2:1)), 0, c);
        f_div(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = (s_lf ? 2U : 1U); n < (unsigned int)g_colors; n++)
        {
            fg_to_float((long)n, 0, f);
            f_mul16(f, m, f);
            f_exp14(f, l);
            limit = (unsigned long)float_to_fg(l, 0) + s_lf;
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (Byte)n;
            }
        }
    }
    else
    {
        s_lf = 0 - g_log_map_flag;
        if (s_lf >= (unsigned long)g_log_map_table_max_size)
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        fg_to_float((long)(g_log_map_table_max_size-s_lf), 0, m);
        f_sqrt14(m, m);
        fg_to_float((long)(g_colors-2), 0, c);
        f_div(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = 2U; n < (unsigned int)g_colors; n++)
        {
            fg_to_float((long)n, 0, f);
            f_mul16(f, m, f);
            f_mul16(f, f, l);
            limit = (unsigned long)(float_to_fg(l, 0) + s_lf);
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (Byte)n;
            }
        }
    }
    g_log_map_table[0] = 0;
    if (g_log_map_flag != -1)
    {
        for (unsigned long sp_top = 1U; sp_top < (unsigned long)g_log_map_table_max_size; sp_top++)   // spread top to incl unused colors
        {
            if (g_log_map_table[sp_top] > g_log_map_table[sp_top-1])
            {
                g_log_map_table[sp_top] = (Byte)(g_log_map_table[sp_top-1]+1);
            }
        }
    }
}

long log_table_calc(long color_iter)
{
    long ret = 0;

    if (g_log_map_flag == 0 && !g_iteration_ranges_len)   // Oops, how did we get here?
    {
        return color_iter;
    }
    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        return g_log_map_table[(long)std::min(color_iter, g_log_map_table_max_size)];
    }

    if (g_log_map_flag > 0)
    {
        // new log function
        if ((unsigned long)color_iter <= s_lf + 1)
        {
            ret = 1;
        }
        else if ((color_iter - s_lf)/std::log(static_cast<double>(color_iter - s_lf)) <= s_mlf)
        {
            ret = (long)(color_iter - s_lf);
        }
        else
        {
            ret = (long)(s_mlf * std::log(static_cast<double>(color_iter - s_lf))) + 1;
        }
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        if (color_iter == 0)
        {
            ret = 1;
        }
        else
        {
            ret = (long)(s_mlf * std::log(static_cast<double>(color_iter))) + 1;
        }
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        if ((unsigned long)color_iter <= s_lf)
        {
            ret = 1;
        }
        else if ((unsigned long)(color_iter - s_lf) <= (unsigned long)(s_mlf * s_mlf))
        {
            ret = (long)(color_iter - s_lf + 1);
        }
        else
        {
            ret = (long)(s_mlf * std::sqrt(static_cast<double>(color_iter - s_lf))) + 1;
        }
    }
    return ret;
}
