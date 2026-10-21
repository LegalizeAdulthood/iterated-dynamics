// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Inverse Julia and orbit calculation, plotting, and MIIM queues.
//
#include "engine/jiim.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/random_seed.h"
#include "engine/VideoInfo.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/diskvid.h"
#include "ui/stop_msg.h"

#include <cmath>
#include <stdexcept>

using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

enum class SecretMode
{
    UNMODIFIED_RANDOM_WALK = 0,
    ALWAYS_GO_ONE_DIRECTION = 1,
    GO_ONE_DIR_DRAW_OTHER_DIR = 2,
    GO_NEGATIVE_IF_MAX_COLOR = 4,
    GO_POSITIVE_IF_MAX_COLOR = 5,
    SIX = 6,
    SEVEN = 7,
    GO_IN_LONG_ZIG_ZAGS = 8,
    RANDOM_RUN = 9,
};

bool g_has_inverse{};                                 // can switch between corresponding Julia/Mandelbrot set
DComplex g_julia_c{JULIA_C_NOT_SET, JULIA_C_NOT_SET}; // C-plane location for Julia set
DComplex g_save_c{-3000.0, -3000.0};                  //

static int s_circle_x{};                  // circle routines from Dr. Dobbs June 1990
static int s_circle_y{};                  // circle center point
static unsigned int s_x_aspect{};         //
static unsigned int s_y_aspect{};         //
static long s_list_front{};               // head, tail, size of MIIM Queue
static long s_list_back{};                //
static long s_list_size{};                //
static long s_l_size{};                   // how many in queue (now, ever)
static long s_l_max{};                    //
static float s_lucky_x{};                 //
static float s_lucky_y{};                 //

static void set_aspect(double aspect)
{
    s_x_aspect = 0;
    s_y_aspect = 0;
    aspect = std::abs(aspect);
    if (aspect != 1.0)
    {
        if (aspect > 1.0)
        {
            s_y_aspect = static_cast<unsigned int>(65536.0 / aspect);
        }
        else
        {
            s_x_aspect = static_cast<unsigned int>(65536.0 * aspect);
        }
    }
}

// Clipping target for the legacy g_plot function-pointer output.
static InverseJuliaPlot s_plot{};

static void c_put_color(const int x, const int y, const int color)
{
    // avoid writing outside window
    if (x < s_plot.corner_x || y < s_plot.corner_y || x >= s_plot.corner_x + s_plot.width ||
        y >= s_plot.corner_y + s_plot.height)
    {
        return;
    }
    if (y >= s_plot.bottom) // avoid overwriting coords
    {
        return;
    }
    if (s_plot.protect_image) // avoid overwriting fractal
    {
        if (0 <= x && x < g_logical_screen.x_dots && 0 <= y && y < g_logical_screen.y_dots)
        {
            return;
        }
    }
    g_put_color(x, y, color);
}

static int c_get_color(const int x, const int y)
{
    // avoid reading outside window
    if (x < s_plot.corner_x || y < s_plot.corner_y || x >= s_plot.corner_x + s_plot.width ||
        y >= s_plot.corner_y + s_plot.height)
    {
        return 1000;
    }
    if (y >= s_plot.bottom) // avoid over reading coords
    {
        return 1000;
    }
    if (s_plot.protect_image) // avoid over reading fractal
    {
        if (0 <= x && x < g_logical_screen.x_dots && 0 <= y && y < g_logical_screen.y_dots)
        {
            return 1000;
        }
    }
    return get_color(x, y);
}

static void circle_plot(const int x, const int y, const int color)
{
    if (s_x_aspect == 0)
    {
        if (s_y_aspect == 0)
        {
            c_put_color(x+s_circle_x, y+s_circle_y, color);
        }
        else
        {
            c_put_color(x + s_circle_x,
                static_cast<short>(
                    s_circle_y + ((static_cast<long>(y) * static_cast<long>(s_y_aspect)) >> 16)),
                color);
        }
    }
    else
    {
        c_put_color(
            static_cast<int>(s_circle_x + ((static_cast<long>(x) * static_cast<long>(s_x_aspect)) >> 16)),
            y + s_circle_y, color);
    }
}

static void plot8(const int x, const int y, const int color)
{
    circle_plot(x, y, color);
    circle_plot(-x, y, color);
    circle_plot(x, -y, color);
    circle_plot(-x, -y, color);
    circle_plot(y, x, color);
    circle_plot(-y, x, color);
    circle_plot(y, -x, color);
    circle_plot(-y, -x, color);
}

static void circle(const int radius, const int color)
{
    int x = 0;
    int y = radius << 1;
    int sum = 0;

    while (x <= y)
    {
        if (!(x & 1))       // plot if x is even
        {
            plot8(x >> 1, (y+1) >> 1, color);
        }
        sum += (x << 1) + 1;
        x++;
        if (sum > 0)
        {
            sum -= (y << 1) - 1;
            y--;
        }
    }
}

// MIIM section:
//
// Global variables and service functions used for computing
// MIIM Julias will be grouped here (and shared by code in LORENZ.C)
//
// Queue/Stack Section:
//
// Defines a buffer that can be used as a FIFO queue or LIFO stack.
//
int queue_empty()            // True if NO points remain in queue
{
    return s_list_front == s_list_back;
}

int queue_full_almost()       // True if room for ONE more point in queue
{
    return (s_list_front + 2) % s_list_size == s_list_back;
}

void clear_queue()
{
    s_l_max = 0;
    s_l_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

// Queue functions for MIIM julia:
bool init_queue(const unsigned long request)
{
    if (driver_is_disk())
    {
        stop_msg("Don't try this in disk video mode, kids...\n");
        s_list_size = 0;
        return false;
    }

    for (s_list_size = request; s_list_size > 1024; s_list_size /= 2)
    {
        switch (common_start_disk(s_list_size * 8, 1, 256))
        {
        case 0:                        // success
            s_list_back = 0;
            s_list_front = 0;
            s_l_max = 0;
            s_l_size = 0;
            return true;
        case -1:
            continue;                   // try smaller queue size
        case -2:
            s_list_size = 0;               // cancelled by user
            return false;
        }
    }

    // failed to get memory for MIIM Queue
    s_list_size = 0;
    return false;
}

void free_queue()
{
    end_disk();
    s_l_max = 0;
    s_l_size = 0;
    s_list_size = 0;
    s_list_back = 0;
    s_list_front = 0;
}

int push_float(float x, float y)
{
    if ((s_list_front + 1) % s_list_size != s_list_back)
    {
        if (to_mem_disk(8*s_list_front, sizeof(x), &x)
            && to_mem_disk(8*s_list_front +sizeof(x), sizeof(y), &y))
        {
            s_list_front = (s_list_front + 1) % s_list_size;
            if (++s_l_size > s_l_max)
            {
                s_l_max   = s_l_size;
                s_lucky_x = x;
                s_lucky_y = y;
            }
            return 1;
        }
    }
    return 0;                    // fail
}

DComplex pop_float()
{
    DComplex pop;
    float pop_x;
    float pop_y;

    if (!queue_empty())
    {
        s_list_front--;
        if (s_list_front < 0)
        {
            s_list_front = s_list_size - 1;
        }
        if (from_mem_disk(8*s_list_front, sizeof(pop_x), &pop_x)
            && from_mem_disk(8*s_list_front +sizeof(pop_x), sizeof(pop_y), &pop_y))
        {
            pop.x = pop_x;
            pop.y = pop_y;
            --s_l_size;
        }
        return pop;
    }
    pop.x = 0;
    pop.y = 0;
    return pop;
}

int enqueue_float(const float x, const float y)
{
    return push_float(x, y);
}

DComplex dequeue_float()
{
    DComplex out;
    float out_x;
    float out_y;

    if (s_list_back != s_list_front)
    {
        if (from_mem_disk(8*s_list_back, sizeof(out_x), &out_x)
            && from_mem_disk(8*s_list_back +sizeof(out_x), sizeof(out_y), &out_y))
        {
            s_list_back = (s_list_back + 1) % s_list_size;
            out.x = out_x;
            out.y = out_y;
            s_l_size--;
        }
        return out;
    }
    out.x = 0;
    out.y = 0;
    return out;
}

InverseJulia::InverseJulia(const JIIMType which) :
    m_which{which}
{
}

void InverseJulia::reset(const DComplex point, const bool use_miim)
{
    if (use_miim && s_list_size == 0)
    {
        throw std::logic_error{"MIIM calculation requires an initialized queue"};
    }
    m_c = point;
    m_use_miim = use_miim;
    m_actively_computing = true;
    m_iter = 1;
    m_old_x = -1;
    m_old_y = -1;
    g_old_z = {};
    g_init = m_c;
    if (m_which == JIIMType::JIIM && m_use_miim)
    {
        const DComplex s = sqrt(1 - 4 * m_c.x, -4 * m_c.y);
        clear_queue();
        m_max_hits = 1;
        enqueue_float(static_cast<float>((1 + s.x) / 2), static_cast<float>(s.y / 2));
        enqueue_float(static_cast<float>((1 - s.x) / 2), static_cast<float>(-s.y / 2));
    }
    if (m_which == JIIMType::ORBIT)
    {
        per_pixel();
    }
    m_init = g_init;
    m_old_z = g_old_z;
    m_new_z = g_new_z;
}

bool InverseJulia::active() const
{
    return m_actively_computing;
}

bool InverseJulia::iterate(const InverseJuliaPlot &plot)
{
    ValueSaver saved_plot{g_plot};
    if (m_which == JIIMType::ORBIT)
    {
        g_plot = c_put_color;
    }
    s_plot = plot;
    if (plot.circles)
    {
        set_aspect(plot.aspect);
    }
    g_old_z = m_old_z;
    g_new_z = m_new_z;
    g_init = m_init;
    if (m_which == JIIMType::JIIM)
    {
        if (iterate_jiim(plot))
        {
            return false;
        }
    }
    else
    {
        iterate_orbit(plot);
    }

    if (m_which == JIIMType::ORBIT || m_iter > 10)
    {
        if (!plot.circles && !plot.lines)
        {
            c_put_color(m_x, m_y, m_color);
        }
        if (plot.circles)
        {
            s_circle_x = m_x;
            s_circle_y = m_y;
            circle(static_cast<int>(plot.zoom * (plot.width >> 1) / m_iter), m_color);
        }
        if (plot.lines && m_x > 0 && m_y > 0 && m_old_x > 0 && m_old_y > 0)
        {
            driver_draw_line(m_x, m_y, m_old_x, m_old_y, m_color);
        }
        m_old_x = m_x;
        m_old_y = m_y;
    }
    g_old_z = g_new_z;

    m_old_z = g_old_z;
    m_new_z = g_new_z;
    return true;
}

bool InverseJulia::iterate_jiim(const InverseJuliaPlot &plot)
{
    if (!g_has_inverse)
    {
        return true;
    }
    // If we have MIIM queue allocated, then use MIIM method.
    if (m_use_miim)
    {
        if (queue_empty())
        {
            if (m_max_hits < g_colors - 1 //
                && m_max_hits < 5         //
                && (s_lucky_x != 0.0 || s_lucky_y != 0.0))
            {
                s_l_max = 0;
                s_l_size = 0;
                g_new_z.x = s_lucky_x;
                g_old_z.x = s_lucky_x;
                g_new_z.y = s_lucky_y;
                g_old_z.y = s_lucky_y;
                s_lucky_y = 0.0f;
                s_lucky_x = 0.0f;
                for (int i = 0; i < 199; i++)
                {
                    g_old_z = sqrt(g_old_z.x - m_c.x, g_old_z.y - m_c.y);
                    g_new_z = sqrt(g_new_z.x - m_c.x, g_new_z.y - m_c.y);
                    enqueue_float(static_cast<float>(g_new_z.x), static_cast<float>(g_new_z.y));
                    enqueue_float(static_cast<float>(-g_old_z.x), static_cast<float>(-g_old_z.y));
                }
                m_max_hits++;
            }
            else
            {
                return true;
            }
        }

        g_old_z = dequeue_float();

        m_x = static_cast<int>(g_old_z.x * plot.x_factor * plot.zoom + plot.x_offset);
        m_y = static_cast<int>(g_old_z.y * plot.y_factor * plot.zoom + plot.y_offset);
        m_color = c_get_color(m_x, m_y);
        if (m_color < m_max_hits)
        {
            c_put_color(m_x, m_y, m_color + 1);
            g_new_z = sqrt(g_old_z.x - m_c.x, g_old_z.y - m_c.y);
            enqueue_float(static_cast<float>(g_new_z.x), static_cast<float>(g_new_z.y));
            enqueue_float(static_cast<float>(-g_new_z.x), static_cast<float>(-g_new_z.y));
        }
    }
    else
    {
        // not MIIM code.
        g_old_z -= m_c;
        m_r = g_old_z.x * g_old_z.x + g_old_z.y * g_old_z.y;
        if (m_r > 10.0)
        {
            g_old_z.y = 0.0;
            g_old_z.x = 0.0; // avoids math error
            m_iter = 1;
            m_r = 0;
        }
        m_iter++;
        m_color = (m_count++ >> 5) % g_colors; // chg color every 32 pts
        if (m_color == 0)
        {
            m_color = 1;
        }

        //       r = sqrt(old.x*old.x + old.y*old.y); calculated above
        m_r = std::sqrt(m_r);
        g_new_z.x = std::sqrt(std::abs((m_r + g_old_z.x) / 2));
        if (g_old_z.y < 0)
        {
            g_new_z.x = -g_new_z.x;
        }

        g_new_z.y = std::sqrt(std::abs((m_r - g_old_z.x) / 2));

        switch (static_cast<SecretMode>(plot.secret_mode))
        {
        case SecretMode::UNMODIFIED_RANDOM_WALK:
        default:
            if (random_int(2))
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
            }
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;

        case SecretMode::ALWAYS_GO_ONE_DIRECTION:
            if (m_c.y < 0)
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
            }
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;

        case SecretMode::GO_ONE_DIR_DRAW_OTHER_DIR:
            if (m_c.y < 0)
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
            }
            m_x = static_cast<int>(-g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(-g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;

        case SecretMode::GO_NEGATIVE_IF_MAX_COLOR:
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            if (c_get_color(m_x, m_y) == g_colors - 1)
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
                m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
                m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            }
            break;

        case SecretMode::GO_POSITIVE_IF_MAX_COLOR:
            g_new_z.x = -g_new_z.x;
            g_new_z.y = -g_new_z.y;
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            if (c_get_color(m_x, m_y) == g_colors - 1)
            {
                m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
                m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            }
            break;

        case SecretMode::SEVEN:
            if (m_c.y < 0)
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
            }
            m_x = static_cast<int>(-g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(-g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            if (m_iter > 10)
            {
                if (!plot.circles && !plot.lines)
                {
                    c_put_color(m_x, m_y, m_color);
                }
                if (plot.circles)
                {
                    s_circle_x = m_x;
                    s_circle_y = m_y;
                    circle(static_cast<int>(plot.zoom * (plot.width >> 1) / m_iter), m_color);
                }
                if (plot.lines && m_x > 0 && m_y > 0 && m_old_x > 0 && m_old_y > 0)
                {
                    driver_draw_line(m_x, m_y, m_old_x, m_old_y, m_color);
                }
                m_old_x = m_x;
                m_old_y = m_y;
            }
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;

        case SecretMode::GO_IN_LONG_ZIG_ZAGS:
            if (m_ran_cnt >= 300)
            {
                m_ran_cnt = -300;
            }
            if (m_ran_cnt < 0)
            {
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
            }
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;

        case SecretMode::RANDOM_RUN:
            switch (m_ran_dir)
            {
            case 0: // go random direction for a while
                if (random_int(2))
                {
                    g_new_z.x = -g_new_z.x;
                    g_new_z.y = -g_new_z.y;
                }
                if (++m_ran_cnt > 1024)
                {
                    m_ran_cnt = 0;
                    if (random_int(2))
                    {
                        m_ran_dir = 1;
                    }
                    else
                    {
                        m_ran_dir = -1;
                    }
                }
                break;
            case 1: // now go negative dir for a while
                g_new_z.x = -g_new_z.x;
                g_new_z.y = -g_new_z.y;
                // fall through
            case -1: // now go positive dir for a while
                if (++m_ran_cnt > 512)
                {
                    m_ran_cnt = 0;
                    m_ran_dir = 0;
                }
                break;
            }
            m_x = static_cast<int>(g_new_z.x * plot.x_factor * plot.zoom + plot.x_offset);
            m_y = static_cast<int>(g_new_z.y * plot.y_factor * plot.zoom + plot.y_offset);
            break;
        }
    }

    return false;
}

void InverseJulia::iterate_orbit(const InverseJuliaPlot &plot)
{
    if (m_iter < g_max_iterations)
    {
        m_color = static_cast<int>(m_iter) % g_colors;
        m_x = static_cast<int>((g_old_z.x - g_init.x) * plot.x_factor * 3 * plot.zoom + plot.x_offset);
        m_y = static_cast<int>((g_old_z.y - g_init.y) * plot.y_factor * 3 * plot.zoom + plot.y_offset);
        if (orbit_calc())
        {
            m_iter = g_max_iterations;
        }
        else
        {
            m_iter++;
        }
    }
    else
    {
        m_y = -1;
        m_x = -1;
        m_actively_computing = false;
    }
}

} // namespace id::engine
