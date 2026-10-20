// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/Diffusion.h"

#include "engine/calcfrac.h"
#include "engine/work_list.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace id::engine
{

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

unsigned long Diffusion::limit() const
{
    return m_limit;
}

bool Diffusion::select_pixel()
{
    g_passes = Passes::DIFFUSION;
    if (!m_initialized)
    {
        initialize();
    }

    if (done())
    {
        return false;
    }
    if (!m_pixel_selected)
    {
        if (m_counters_processed == DIFFUSION_COUNTERS_PER_CHUNK)
        {
            m_counters_processed = 0;
            return false;
        }
        if (m_tile_x == 0 && m_tile_y == 0)
        {
            initialize_counter();
        }
        m_selected_pixel = {
            g_i_start_pt.x + m_orig_col + m_tile_x * m_size, g_i_start_pt.y + m_orig_row + m_tile_y * m_size};
        m_pixel_selected = true;
    }
    g_col = m_selected_pixel.x;
    g_row = m_selected_pixel.y;
    g_reset_periodicity = true;
    return true;
}

math::Point2i Diffusion::selected_pixel() const
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected diffusion pixel"};
    }
    return m_selected_pixel;
}

void Diffusion::complete_pixel(const int color)
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected diffusion pixel to complete"};
    }
    g_col = m_selected_pixel.x;
    g_row = m_selected_pixel.y;
    g_color = color;
    g_reset_periodicity = false;
    if (m_fill_size == 0)
    {
        g_plot(g_col, g_row, color);
    }
    else if (m_tile_x == m_nx || m_tile_y == m_ny)
    {
        plot_block_lim(g_col, g_row, m_fill_size, color);
    }
    else
    {
        plot_block(g_col, g_row, m_fill_size, color);
    }
    m_pixel_selected = false;

    if (++m_tile_y == m_ny + (m_orig_row < m_rem_y ? 1 : 0))
    {
        m_tile_y = 0;
        if (++m_tile_x == m_nx + (m_orig_col < m_rem_x ? 1 : 0))
        {
            m_tile_x = 0;
            ++m_counter;
            ++m_counters_processed;
            update_phase();
        }
    }
}

void Diffusion::begin_chunk()
{
    m_counters_processed = 0;
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

void Diffusion::initialize_counter()
{
    count_to_int(m_counter, m_orig_col, m_orig_row, m_dif_offset);
    m_fill_size = 0;
    if (m_phase == Phase::FIRST_HALF && g_fill_color != 0)
    {
        m_fill_size = 1 << (static_cast<int>(m_bits - static_cast<int>(std::log(m_counter + 0.5) / LOG2) - 1) / 2);
    }
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
