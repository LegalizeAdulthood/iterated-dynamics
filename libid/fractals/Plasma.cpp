// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/Plasma.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/fractals.h"
#include "engine/id_data.h"
#include "engine/random_seed.h"
#include "math/rand15.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/sized_types.h"
#include "ui/diskvid.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/video.h"

#include <algorithm>
#include <cstdlib>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::fractals
{

using PlotFn = void(*)(int, int, int);

static constexpr U16 (*GET_COLOR)(int x, int y){
    [](int x, int y) { return static_cast<U16>(get_color(x, y)); }};

static void set_plasma_palette()
{
    if (g_map_specified || g_colors_preloaded)
    {
        return;    // map= specified
    }

    static const Byte Red[3]   = { 255, 0, 0 };
    static const Byte Green[3] = { 0, 255, 0 };
    static const Byte Blue[3]  = { 0,  0, 255 };

    g_dac_box[0][0] = 0;
    g_dac_box[0][1] = 0;
    g_dac_box[0][2] = 0;
    for (int i = 1; i <= 85; i++)
    {
        g_dac_box[i][0] = static_cast<Byte>((i * Green[0] + (86 - i) * Blue[0]) / 85);
        g_dac_box[i][1] = static_cast<Byte>((i * Green[1] + (86 - i) * Blue[1]) / 85);
        g_dac_box[i][2] = static_cast<Byte>((i * Green[2] + (86 - i) * Blue[2]) / 85);

        g_dac_box[i+85][0] = static_cast<Byte>((i * Red[0] + (86 - i) * Green[0]) / 85);
        g_dac_box[i+85][1] = static_cast<Byte>((i * Red[1] + (86 - i) * Green[1]) / 85);
        g_dac_box[i+85][2] = static_cast<Byte>((i * Red[2] + (86 - i) * Green[2]) / 85);
        g_dac_box[i+170][0] = static_cast<Byte>((i * Blue[0] + (86 - i) * Red[0]) / 85);
        g_dac_box[i+170][1] = static_cast<Byte>((i * Blue[1] + (86 - i) * Red[1]) / 85);
        g_dac_box[i+170][2] = static_cast<Byte>((i * Blue[2] + (86 - i) * Red[2]) / 85);
    }
    spin_dac(0, 1);
}

//**************** standalone engine for "plasma" *******************

// returns a random 16 bit value that is never 0
static U16 rand16()
{
    U16 value = static_cast<U16>(RAND15());
    value <<= 1;
    value = static_cast<U16>(value + (RAND15() & 1));
    value = std::max<U16>(value, 1U);
    return value;
}

static void put_pot(int x, int y, U16 color)
{
    color = std::max<U16>(color, 1U);
    g_put_color(x, y, color >> 8 ? color >> 8 : 1);  // don't write 0
    /* we don't write this if driver_diskp() because the above putcolor
          was already a "writedisk" in that case */
    if (!driver_is_disk())
    {
        disk_write_pixel(x+g_logical_screen_x_offset, y+g_logical_screen_y_offset, color >> 8);    // upper 8 bits
    }
    disk_write_pixel(x+g_logical_screen_x_offset, y+g_screen_y_dots+g_logical_screen_y_offset, color&255); // lower 8 bits
}

// fixes border
static void put_pot_border(int x, int y, U16 color)
{
    if (x == 0 || y == 0 || x == g_logical_screen_x_dots - 1 || y == g_logical_screen_y_dots - 1)
    {
        color = static_cast<U16>(g_outside_color);
    }
    put_pot(x, y, color);
}

// fixes border
static void put_color_border(int x, int y, int color)
{
    if (x == 0 || y == 0 || x == g_logical_screen_x_dots - 1 || y == g_logical_screen_y_dots - 1)
    {
        color = g_outside_color;
    }
    color = std::max(color, 1);
    g_put_color(x, y, color);
}

static U16 get_pot(int x, int y)
{
    U16 color = static_cast<U16>(disk_read_pixel(x + g_logical_screen_x_offset, y + g_logical_screen_y_offset));
    color = static_cast<U16>((color << 8) +
        static_cast<U16>(
            disk_read_pixel(x + g_logical_screen_x_offset, y + g_screen_y_dots + g_logical_screen_y_offset)));
    return color;
}

Plasma::Plasma() :
    m_saved_potential_flag(g_potential_flag),
    m_saved_potential_16_bit(g_potential_16bit),
    m_algo(g_params[1] == 0.0 ? Algorithm::OLD : Algorithm::NEW),
    m_get_pix{GET_COLOR}
{
    m_i_param_x = static_cast<int>(g_params[0] * 8);
    if (g_param_z1.x <= 0.0)
    {
        m_i_param_x = 0;
    }
    if (g_param_z1.x >= 100)
    {
        m_i_param_x = 800;
    }
    g_params[0] = static_cast<double>(m_i_param_x) / 8.0;  // let user know what was used
    // limit parameter values
    g_params[1] = std::clamp(g_params[1], 0.0, 1.0);
    g_params[2] = std::clamp(g_params[2], 0.0, 1.0);
    g_params[3] = std::clamp(g_params[3], 0.0, 1.0);

    if (!g_random_seed_flag && g_params[2] == 1)
    {
        --g_random_seed;
    }
    if (g_params[2] != 0 && g_params[2] != 1)
    {
        g_random_seed = static_cast<int>(g_params[2]);
    }
    m_max_plasma = static_cast<U16>(g_params[3]);  // max_plasma is used as a flag for potential

    if (m_max_plasma != 0)
    {
        if (pot_start_disk() >= 0)
        {
            m_max_plasma = 0xFFFF;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = reinterpret_cast<PlotFn>(put_pot_border);
            }
            else
            {
                g_plot    = reinterpret_cast<PlotFn>(put_pot);
            }
            m_get_pix =  get_pot;
        }
        else
        {
            m_max_plasma = 0;        // can't do potential (startdisk failed)
            g_params[3]   = 0;
            if (g_outside_color >= COLOR_BLACK)
            {
                g_plot    = put_color_border;
            }
            else
            {
                g_plot    = g_put_color;
            }
            m_get_pix = GET_COLOR;
        }
    }
    else
    {
        if (g_outside_color >= COLOR_BLACK)
        {
            g_plot    = put_color_border;
        }
        else
        {
            g_plot    = g_put_color;
        }
            m_get_pix = GET_COLOR;
    }
    set_random_seed();

    if (g_colors == 256)                     // set the (256-color) palette
    {
        set_plasma_palette();             // skip this if < 256 colors
    }
    if (g_colors > 16)
    {
        m_shift_value = 18;
    }
    else if (g_colors > 4)
    {
        m_shift_value = 22;
    }
    else if (g_colors > 2)
    {
        m_shift_value = 24;
    }
    else
    {
        m_shift_value = 25;
    }
    if (m_max_plasma != 0)
    {
        m_shift_value = 10;
    }

    if (m_max_plasma == 0)
    {
        m_p_colors = std::min(g_colors, 256);
        for (U16 &elem : m_rnd)
        {
            elem = static_cast<U16>(1 + ((RAND15() / m_p_colors * (m_p_colors - 1)) >> (m_shift_value - 11)));
        }
    }
    else
    {
        for (U16 &elem : m_rnd)
        {
            elem = rand16();
        }
    }
    if (g_debug_flag == DebugFlags::PREVENT_PLASMA_RANDOM)
    {
        for (U16 &elem : m_rnd)
        {
            elem = 1;
        }
    }

    g_plot(0,      0,  m_rnd[0]);
    g_plot(g_logical_screen_x_dots-1,      0,  m_rnd[1]);
    g_plot(g_logical_screen_x_dots-1, g_logical_screen_y_dots-1,  m_rnd[2]);
    g_plot(0, g_logical_screen_y_dots-1,  m_rnd[3]);

    m_recur_level = 0;

    if (m_algo == Algorithm::NEW)
    {
        m_level = 1;
        m_k = 1;
        m_scale = 1;
    }

    m_subdivs.push_back(Subdivision{0, 0, g_logical_screen_x_dots - 1, g_logical_screen_y_dots - 1, 0});
}

Plasma::~Plasma()
{
    g_plot = g_put_color;
    m_get_pix = GET_COLOR;
}

bool Plasma::done() const
{
    return m_done;
}

U16 Plasma::adjust(int xa, int ya, int x, int y, int xb, int yb, int scale)
{
    S32 pseudorandom = m_i_param_x * (RAND15() - 16383);
    pseudorandom = pseudorandom * scale;
    pseudorandom = pseudorandom >> m_shift_value;
    pseudorandom = ((static_cast<S32>(m_get_pix(xa, ya)) + static_cast<S32>(m_get_pix(xb, yb)) +1) >> 1)+pseudorandom;
    if (m_max_plasma == 0)
    {
        if (pseudorandom >= m_p_colors)
        {
            pseudorandom = m_p_colors-1;
        }
    }
    else if (pseudorandom >= static_cast<S32>(m_max_plasma))
    {
        pseudorandom = m_max_plasma;
    }
    pseudorandom = std::max<S32>(pseudorandom, 1);
    g_plot(x, y, static_cast<U16>(pseudorandom));
    return static_cast<U16>(pseudorandom);
}

void Plasma::subdivide()
{
    const Subdivision sd{m_subdivs.back()};
    m_subdivs.pop_back();
    if (sd.x2 - sd.x1 < 2 && sd.y2 - sd.y1 < 2)
    {
        return;
    }
    const int level{sd.level + 1};
    const int scale = static_cast<int>(320L >> level);

    int x = (sd.x1 + sd.x2) >> 1;
    int y = (sd.y1 + sd.y2) >> 1;
    S32 v = m_get_pix(x, sd.y1);
    if (v == 0)
    {
        v = adjust(sd.x1, sd.y1, x, sd.y1, sd.x2, sd.y1, scale);
    }
    S32 i = v;
    v = m_get_pix(sd.x2, y);
    if (v == 0)
    {
        v = adjust(sd.x2, sd.y1, sd.x2, y, sd.x2, sd.y2, scale);
    }
    i += v;
    v = m_get_pix(x, sd.y2);
    if (v == 0)
    {
        v = adjust(sd.x1, sd.y2, x, sd.y2, sd.x2, sd.y2, scale);
    }
    i += v;
    v = m_get_pix(sd.x1, y);
    if (v == 0)
    {
        v = adjust(sd.x1, sd.y1, sd.x1, y, sd.x1, sd.y2, scale);
    }
    i += v;

    if (m_get_pix(x, y) == 0)
    {
        g_plot(x, y, static_cast<U16>((i + 2) >> 2));
    }

    m_subdivs.push_back(Subdivision{sd.x1, y, x, sd.y2, level});
    m_subdivs.push_back(Subdivision{x, y, sd.x2, sd.y2, level});
    m_subdivs.push_back(Subdivision{x, sd.y1, sd.x2, y, level});
    m_subdivs.push_back(Subdivision{sd.x1, sd.y1, x, y, level});
}

void Plasma::subdivide_new(int x1, int y1, int x2, int y2, int level)
{
    m_scale = static_cast<int>(320L >> level);
    m_sub_y.top = 2;
    m_sub_y.value[0] = y2;
    int ny = y2;
    m_sub_y.value[2] = y1;
    int ny1 = y1;
    m_sub_y.level[0] = 0;
    m_sub_y.level[1] = 1;
    m_sub_y.level[2] = 0;
    m_sub_y.value[1] = (ny1 + ny) >> 1;
    int y = m_sub_y.value[1];

    while (m_sub_y.top >= 1)
    {
        while (m_sub_y.level[m_sub_y.top-1] < level)
        {
            //     1.  Create new entry at top of the stack
            //     2.  Copy old top value to new top value.
            //            This is largest y value.
            //     3.  Smallest y is now old mid-point
            //     4.  Set new mid-point recursion level
            //     5.  New mid-point value is average
            //            of largest and smallest

            m_sub_y.top++;
            m_sub_y.value[m_sub_y.top] = m_sub_y.value[m_sub_y.top-1];
            ny1  = m_sub_y.value[m_sub_y.top];
            ny   = m_sub_y.value[m_sub_y.top-2];
            m_sub_y.level[m_sub_y.top] = m_sub_y.level[m_sub_y.top-1];
            m_sub_y.value[m_sub_y.top-1]   = (ny1 + ny) >> 1;
            y    = m_sub_y.value[m_sub_y.top-1];
            m_sub_y.level[m_sub_y.top-1]   = std::max(m_sub_y.level[m_sub_y.top], m_sub_y.level[m_sub_y.top - 2]) + 1;
        }
        m_sub_x.top = 2;
        m_sub_x.value[0] = x2;
        int nx = x2;
        m_sub_x.value[2] = x1;
        int nx1 = x1;
        m_sub_x.level[2] = 0;
        m_sub_x.level[0] = 0;
        m_sub_x.level[1] = 1;
        m_sub_x.value[1] = (nx1 + nx) >> 1;
        int x = m_sub_x.value[1];

        while (m_sub_x.top >= 1)
        {
            while (m_sub_x.level[m_sub_x.top-1] < level)
            {
                m_sub_x.top++; // move the top ofthe stack up 1
                m_sub_x.value[m_sub_x.top] = m_sub_x.value[m_sub_x.top-1];
                nx1  = m_sub_x.value[m_sub_x.top];
                nx   = m_sub_x.value[m_sub_x.top-2];
                m_sub_x.level[m_sub_x.top] = m_sub_x.level[m_sub_x.top-1];
                m_sub_x.value[m_sub_x.top-1]   = (nx1 + nx) >> 1;
                x    = m_sub_x.value[m_sub_x.top-1];
                m_sub_x.level[m_sub_x.top-1]   = std::max(m_sub_x.level[m_sub_x.top], m_sub_x.level[m_sub_x.top - 2]) + 1;
            }

            S32 i = m_get_pix(nx, y);
            if (i == 0)
            {
                i = adjust(nx, ny1, nx, y, nx, ny, m_scale);
            }
            S32 v = i;
            i = m_get_pix(x, ny);
            if (i == 0)
            {
                i = adjust(nx1, ny, x, ny, nx, ny, m_scale);
            }
            v += i;
            if (m_get_pix(x, y) == 0)
            {
                i = m_get_pix(x, ny1);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, x, ny1, nx, ny1, m_scale);
                }
                v += i;
                i = m_get_pix(nx1, y);
                if (i == 0)
                {
                    i = adjust(nx1, ny1, nx1, y, nx1, ny, m_scale);
                }
                v += i;
                g_plot(x, y, static_cast<U16>((v + 2) >> 2));
            }

            if (m_sub_x.level[m_sub_x.top-1] == level)
            {
                m_sub_x.top = m_sub_x.top - 2;
            }
        }

        if (m_sub_y.level[m_sub_y.top-1] == level)
        {
            m_sub_y.top = m_sub_y.top - 2;
        }
    }
}

void Plasma::iterate()
{
    if (m_algo == Algorithm::OLD)
    {
        subdivide();
        m_done = m_subdivs.empty();
    }
    else
    {
        subdivide_new(0, 0, g_logical_screen_x_dots - 1, g_logical_screen_y_dots - 1, m_level);
        if (!m_done)
        {
            m_k *= 2;
            if (m_k  > static_cast<int>(std::max(g_logical_screen_x_dots - 1, g_logical_screen_y_dots - 1)))
            {
                m_done = true;
            }
            m_level++;
        }
    }
}

} // namespace id::fractals
