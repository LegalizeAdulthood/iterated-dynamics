// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/log_map.h"

#include "engine/calcfrac.h"
#include "engine/VideoInfo.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <algorithm>
#include <cmath>

using namespace id::misc;

namespace id::engine
{

long g_log_map_flag{};                     // Logarithmic palette flag: 0 = no
LogMapCalculate g_log_map_fly_calculate{}; // calculate logmap on-the-fly
bool g_log_map_auto_calculate{};           // auto calculate logmap
std::vector<Byte> g_log_map_table;
long g_log_map_table_max_size{};
bool g_log_map_calculate{};

static double s_mlf{};
static unsigned long s_lf{};

static float legacy_log14(const float value)
{
    constexpr float FIXED_SCALE{65536.0F};
    const auto fixed = static_cast<long>(std::log(static_cast<double>(value)) * FIXED_SCALE);
    return static_cast<float>(fixed) / FIXED_SCALE;
}

static float legacy_exp14(const float value)
{
    constexpr float LOG_TWO{0.6931472F};
    constexpr float FIXED_SCALE{65536.0F};
    const int fudge = 23 - static_cast<int>(value / LOG_TWO);
    const float scale = std::ldexp(1.0F, fudge);
    const auto fixed = static_cast<long>(value * FIXED_SCALE);
    const auto ans = static_cast<long>(std::exp(static_cast<double>(fixed) / FIXED_SCALE) * scale);
    return static_cast<float>(ans) / scale;
}

static float legacy_sqrt14(const float value)
{
    return legacy_exp14(legacy_log14(value) / 2.0F);
}

static void setup_legacy_log_table()
{
    if (g_log_map_table.empty())
    {
        return;
    }

    unsigned long prev{};
    unsigned long limit{};
    if (g_log_map_flag > -2)
    {
        s_lf = g_log_map_flag > 1 ? g_log_map_flag : 0;
        if (s_lf >= static_cast<unsigned long>(g_log_map_table_max_size))
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        float m = legacy_log14(static_cast<float>(g_log_map_table_max_size - s_lf));
        const float c = static_cast<float>(g_colors - (s_lf ? 2 : 1));
        m /= c;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = s_lf ? 2U : 1U; n < static_cast<unsigned>(g_colors); n++)
        {
            float f = static_cast<float>(n);
            f *= m;
            const float l = legacy_exp14(f);
            limit = static_cast<unsigned long>(l) + s_lf;
            if (limit > static_cast<unsigned long>(g_log_map_table_max_size) || n == static_cast<unsigned>(g_colors - 1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = static_cast<Byte>(n);
            }
        }
    }
    else
    {
        s_lf = 0 - g_log_map_flag;
        if (s_lf >= static_cast<unsigned long>(g_log_map_table_max_size))
        {
            s_lf = g_log_map_table_max_size - 1;
        }
        float m = legacy_sqrt14(static_cast<float>(g_log_map_table_max_size - s_lf));
        const float c = static_cast<float>(g_colors - 2);
        m /= c;
        for (prev = 1; prev <= s_lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = 2; n < static_cast<unsigned>(g_colors); n++)
        {
            float f = static_cast<float>(n);
            f *= m;
            f *= f;
            limit = static_cast<unsigned long>(f) + s_lf;
            if (limit > static_cast<unsigned long>(g_log_map_table_max_size) || n == static_cast<unsigned>(g_colors - 1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = static_cast<Byte>(n);
            }
        }
    }

    g_log_map_table[0] = 0;
    if (g_log_map_flag != -1)
    {
        for (unsigned long sptop = 1; sptop < static_cast<unsigned long>(g_log_map_table_max_size); sptop++)
        {
            if (g_log_map_table[sptop] > g_log_map_table[sptop - 1])
            {
                g_log_map_table[sptop] = static_cast<Byte>(g_log_map_table[sptop - 1] + 1);
            }
        }
    }
}

/* int LogFlag;
   LogFlag == 1  -- standard log palettes
   LogFlag == -1 -- 'old' log palettes
   LogFlag >  1  -- compress counts < LogFlag into color #1
   LogFlag < -1  -- use quadratic palettes based on square roots && compress
*/
void setup_log_table()
{
    if (!(g_version <= 1920) || g_log_map_fly_calculate == LogMapCalculate::ON_THE_FLY)
    {
        // set up on-the-fly variables
        if (g_log_map_flag > 0)
        {
            // new log function
            s_lf = g_log_map_flag > 1 ? g_log_map_flag : 0;
            if (s_lf >= static_cast<unsigned long>(g_log_map_table_max_size))
            {
                s_lf = g_log_map_table_max_size - 1;
            }
            s_mlf = (g_colors - (s_lf ? 2 : 1)) / std::log(static_cast<double>(g_log_map_table_max_size - s_lf));
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
            if (s_lf >= static_cast<unsigned long>(g_log_map_table_max_size))
            {
                s_lf = g_log_map_table_max_size - 1;
            }
            s_mlf = (g_colors - 2) / std::sqrt(static_cast<double>(g_log_map_table_max_size - s_lf));
        }
    }

    if (g_log_map_calculate)
    {
        return; // LogTable not defined, bail out now
    }

    if (!(g_version <= 1920))
    {
        ValueSaver saved_log_map_calculate{g_log_map_calculate, true}; // turn it on
        for (long i = 0U; i < static_cast<long>(g_log_map_table.size()); i++)
        {
            g_log_map_table[i] = static_cast<Byte>(log_table_calc(i));
        }
        return;
    }

    setup_legacy_log_table();
}

long log_table_calc(const long color_iter)
{
    long ret = 0;

    if (g_log_map_flag == 0 && g_iteration_ranges.empty())   // Oops, how did we get here?
    {
        return color_iter;
    }
    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        return g_log_map_table[std::min(color_iter, g_log_map_table_max_size)];
    }

    if (g_log_map_flag > 0)
    {
        // new log function
        if (static_cast<unsigned long>(color_iter) <= s_lf + 1)
        {
            ret = 1;
        }
        else if ((color_iter - s_lf)/std::log(static_cast<double>(color_iter - s_lf)) <= s_mlf)
        {
            ret = g_version < 2002 ? static_cast<long>(color_iter - s_lf + (s_lf ? 1 : 0)) :
                                      static_cast<long>(color_iter - s_lf);
        }
        else
        {
            ret = static_cast<long>(s_mlf * std::log(static_cast<double>(color_iter - s_lf))) + 1;
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
            ret = static_cast<long>(s_mlf * std::log(static_cast<double>(color_iter))) + 1;
        }
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        if (static_cast<unsigned long>(color_iter) <= s_lf)
        {
            ret = 1;
        }
        else if (color_iter - s_lf <= static_cast<unsigned long>(s_mlf * s_mlf))
        {
            ret = static_cast<long>(color_iter - s_lf + 1);
        }
        else
        {
            ret = static_cast<long>(s_mlf * std::sqrt(static_cast<double>(color_iter - s_lf))) + 1;
        }
    }
    return ret;
}

} // namespace id::engine
