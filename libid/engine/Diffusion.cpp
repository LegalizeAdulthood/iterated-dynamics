// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/Diffusion.h"

#include "engine/calcfrac.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace id::engine
{

using namespace id::fractals;

namespace
{

// lookup tables to avoid too much bit fiddling :
// clang-format off
constexpr int DIF_LA[] =
{
    0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
    2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
    6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
    5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
    1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
    0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
    6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
    6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
    3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
    1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

constexpr int DIF_LB[] =
{
    0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
    6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
    2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
    13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
    11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
    5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
    13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
    11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
    9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
    10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
    0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
    6, 6,14
};
// clang-format on
constexpr double LOG2{0.69314718055994530942};
constexpr int DIFFUSION_COUNTERS_PER_CHUNK{512};

} // namespace

unsigned int Diffusion::bits() const
{
    return m_bits;
}

unsigned long Diffusion::counter() const
{
    return m_counter;
}

bool Diffusion::iterate()
{
    scan();
    return done();
}

unsigned long Diffusion::limit() const
{
    return m_limit;
}

int Diffusion::scan()
{
    g_passes = Passes::DIFFUSION;
    if (!m_initialized)
    {
        initialize();
    }

    for (int processed = 0; processed < DIFFUSION_COUNTERS_PER_CHUNK && !done(); ++processed)
    {
        if (!process_counter())
        {
            return -1;
        }
    }

    return 0;
}

void Diffusion::suspend()
{
    if (done())
    {
        return;
    }
    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x,
        static_cast<int>(m_counter >> 16),    // high,
        static_cast<int>(m_counter & 0xffff), // low order words
        g_work_symmetry);
}

bool Diffusion::calculate_current_pixel()
{
    g_reset_periodicity = true;
    if (calc_type() == -1)
    {
        return false;
    }
    g_reset_periodicity = false;
    return true;
}

bool Diffusion::done() const
{
    return m_phase == Phase::DONE;
}

void Diffusion::initialize()
{
    m_initialized = true;

    // note: the max size of 2048x2048 gives us a 22 bit counter that will
    // fit any 32 bit architecture, the maximum limit for this case would
    // be 65536x65536
    m_bits = static_cast<unsigned>(std::min( //
                                       std::log(static_cast<double>(g_i_stop_pt.y - g_i_start_pt.y + 1)),
                                       std::log(static_cast<double>(g_i_stop_pt.x - g_i_start_pt.x + 1))) /
        LOG2);
    m_bits <<= 1;               // double for two axes
    m_limit = 1UL << m_bits;
    m_size = 1 << (m_bits / 2); // size of the square
    // number of tiles to build in x and y dirs
    m_nx = static_cast<int>(std::floor(static_cast<double>((g_i_stop_pt.x - g_i_start_pt.x + 1) / m_size)));
    m_ny = static_cast<int>(std::floor(static_cast<double>((g_i_stop_pt.y - g_i_start_pt.y + 1) / m_size)));
    // made this to complete the area that is not
    // a square with sides like 2 ** n
    // what is left on the last tile to draw
    m_rem_x = g_i_stop_pt.x - g_i_start_pt.x + 1 - m_nx * m_size;
    m_rem_y = g_i_stop_pt.y - g_i_start_pt.y + 1 - m_ny * m_size;

    if (g_begin_pt.y == g_i_start_pt.y && g_work_pass == 0)
    {
        // if restarting on pan:
        m_counter = 0L;
    }
    else
    {
        // yybegin and passes contain data for resuming the type:
        m_counter = static_cast<long>(static_cast<unsigned>(g_begin_pt.y)) << 16 | static_cast<unsigned>(g_work_pass);
    }

    m_dif_offset = 12 - m_bits / 2; // offset to adjust coordinates
    // (*) for 4 bytes use 16 for 3 use 12 etc.
    update_phase();
}

void Diffusion::plot_block(const int x, const int y, const int size, const int color)
{
    std::memset(m_stack.data(), color, size);
    for (int ty = y; ty < y + size; ty++)
    {
        sym_fill_line(ty, x, x + size - 1, m_stack.data());
    }
}

void Diffusion::plot_block_lim(const int x, const int y, const int size, const int color)
{
    std::memset(m_stack.data(), color, size);
    for (int ty = y; ty < std::min(y + size, g_i_stop_pt.y + 1); ty++)
    {
        sym_fill_line(ty, x, std::min(x + size - 1, g_i_stop_pt.x), m_stack.data());
    }
}

void Diffusion::count_to_int(const unsigned long counter, int &x, int &y, const int dif_offset)
{
    unsigned long t_c = counter;
    x = DIF_LA[t_c & 0xFF];
    y = DIF_LB[t_c & 0xFF];
    t_c >>= 8;
    x <<= 4;
    x += DIF_LA[t_c & 0xFF];
    y <<= 4;
    y += DIF_LB[t_c & 0xFF];
    t_c >>= 8;
    x <<= 4;
    x += DIF_LA[t_c & 0xFF];
    y <<= 4;
    y += DIF_LB[t_c & 0xFF];
    x >>= dif_offset;
    y >>= dif_offset;
}

bool Diffusion::process_counter()
{
    update_phase();
    if (done())
    {
        return true;
    }

    bool completed{};
    if (m_phase == Phase::SECOND_HALF)
    {
        completed = process_point_counter();
    }
    else if (g_fill_color == 0)
    {
        completed = process_dither_counter();
    }
    else
    {
        completed = process_fill_counter();
    }
    if (!completed)
    {
        return false;
    }

    ++m_counter;
    update_phase();
    return true;
}

bool Diffusion::process_dither_counter()
{
    count_to_int(m_counter, m_orig_col, m_orig_row, m_dif_offset);

    m_tile_x = 0;
    g_col = g_i_start_pt.x + m_orig_col; // get the right tiles
    do
    {
        m_tile_y = 0;
        g_row = g_i_start_pt.y + m_orig_row;
        do
        {
            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
            ++m_tile_y;
            g_row += m_size; // next tile
        } while (m_tile_y < m_ny);
        // in the last y tile we may not need to plot the point
        if (m_orig_row < m_rem_y)
        {
            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
        }
        ++m_tile_x;
        g_col += m_size;
    } while (m_tile_x < m_nx);
    // in the last x tiles we may not need to plot the point
    if (m_orig_col < m_rem_x)
    {
        g_row = g_i_start_pt.y + m_orig_row;
        m_tile_y = 0;
        do
        {
            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
            ++m_tile_y;
            g_row += m_size; // next tile
        } while (m_tile_y < m_ny);
        if (m_orig_row < m_rem_y)
        {
            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
        }
    }
    return true;
}

bool Diffusion::process_fill_counter()
{
    // size of the block being filled
    const int sq_size = 1 << (static_cast<int>(m_bits - static_cast<int>(std::log(m_counter + 0.5) / LOG2) - 1) / 2);

    count_to_int(m_counter, m_orig_col, m_orig_row, m_dif_offset);

    m_tile_x = 0;
    do
    {
        m_tile_y = 0;
        do
        {
            g_col = g_i_start_pt.x + m_orig_col + m_tile_x * m_size; // get the right tiles
            g_row = g_i_start_pt.y + m_orig_row + m_tile_y * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            plot_block(g_col, g_row, sq_size, g_color);
            ++m_tile_y;
        } while (m_tile_y < m_ny);
        // in the last tile we may not need to plot the point
        if (m_orig_row < m_rem_y)
        {
            g_row = g_i_start_pt.y + m_orig_row + m_ny * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            plot_block_lim(g_col, g_row, sq_size, g_color);
        }
        ++m_tile_x;
    } while (m_tile_x < m_nx);
    // in the last tile we may not need to plot the point
    if (m_orig_col < m_rem_x)
    {
        g_col = g_i_start_pt.x + m_orig_col + m_nx * m_size;
        m_tile_y = 0;
        do
        {
            g_row = g_i_start_pt.y + m_orig_row + m_tile_y * m_size; // get the right tiles

            if (!calculate_current_pixel())
            {
                return false;
            }
            plot_block_lim(g_col, g_row, sq_size, g_color);
            ++m_tile_y;
        } while (m_tile_y < m_ny);
        // in the last tile we may nnt need to plot the point
        if (m_orig_row < m_rem_y)
        {
            g_row = g_i_start_pt.y + m_orig_row + m_ny * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            plot_block_lim(g_col, g_row, sq_size, g_color);
        }
    }
    return true;
}

bool Diffusion::process_point_counter()
{
    count_to_int(m_counter, m_orig_col, m_orig_row, m_dif_offset);

    m_tile_x = 0;
    do
    {
        m_tile_y = 0;
        do
        {
            g_col = g_i_start_pt.x + m_orig_col + m_tile_x * m_size; // get the right tiles
            g_row = g_i_start_pt.y + m_orig_row + m_tile_y * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
            ++m_tile_y;
        } while (m_tile_y < m_ny);
        // in the last tile we may not need to plot the point
        if (m_orig_row < m_rem_y)
        {
            g_row = g_i_start_pt.y + m_orig_row + m_ny * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
        }
        ++m_tile_x;
    } while (m_tile_x < m_nx);
    // in the last tile we may not need to plot the point
    if (m_orig_col < m_rem_x)
    {
        g_col = g_i_start_pt.x + m_orig_col + m_nx * m_size;
        m_tile_y = 0;
        do
        {
            g_row = g_i_start_pt.y + m_orig_row + m_tile_y * m_size; // get the right tiles

            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
            ++m_tile_y;
        } while (m_tile_y < m_ny);
        if (m_orig_row < m_rem_y)
        {
            g_row = g_i_start_pt.y + m_orig_row + m_ny * m_size;

            if (!calculate_current_pixel())
            {
                return false;
            }
            g_plot(g_col, g_row, g_color);
        }
    }
    return true;
}

void Diffusion::update_phase()
{
    if (m_counter >= m_limit)
    {
        m_phase = Phase::DONE;
    }
    else if (m_counter >= (m_limit >> 1))
    {
        m_phase = Phase::SECOND_HALF;
    }
    else
    {
        m_phase = Phase::FIRST_HALF;
    }
}

} // namespace id::engine
