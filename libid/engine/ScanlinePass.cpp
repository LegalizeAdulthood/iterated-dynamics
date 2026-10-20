// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/ScanlinePass.h"

#include "engine/calcfrac.h"
#include "engine/resume.h"
#include "engine/StandardFractal.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/lyapunov.h"
#include "ui/video.h"

#include <stdexcept>

using namespace id::ui;
using namespace id::fractals;

namespace id::engine
{

ScanlinePass::ScanlinePass(const CalcMode calc_mode) :
    m_calc_mode{calc_mode}
{
}

OnePass::OnePass() :
    ScanlinePass{CalcMode::ONE_PASS}
{
}

TwoPass::TwoPass() :
    ScanlinePass{CalcMode::TWO_PASS}
{
}

void ScanlinePass::start_pass()
{
    m_current_pass = m_calc_mode == CalcMode::TWO_PASS && g_work_pass == 0 ? 1 : 2;
    g_current_pass = m_current_pass;
    m_row = g_begin_pt.y;
    m_col = g_begin_pt.x;
    g_reset_periodicity = true;
    m_standard_calc_active = true;
}

bool ScanlinePass::select_pixel()
{
    if (m_done)
    {
        return false;
    }
    g_passes = Passes::SEQUENTIAL_SCAN;
    g_total_passes = m_calc_mode == CalcMode::ONE_PASS ? 1 : 2;
    if (m_pixel_selected)
    {
        g_row = m_row;
        g_col = m_col;
        g_current_row = m_row;
        return true;
    }
    if (!m_standard_calc_active)
    {
        start_pass();
    }
    g_current_pass = m_current_pass;
    if (select_current_pass_pixel())
    {
        return true;
    }
    if (m_current_pass == 1)
    {
        if (g_num_work_list > 0) // worklist not empty, defer 2nd pass
        {
            add_work_list(
                g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x, g_start_pt.y, 1, g_work_symmetry);
            m_done = true;
            return false;
        }
        g_work_pass = 1;
        g_begin_pt.x = g_start_pt.x;
        g_begin_pt.y = g_start_pt.y;
        start_pass();
        if (select_current_pass_pixel())
        {
            return true;
        }
    }
    m_done = true;
    return false;
}

math::Point2i ScanlinePass::selected_pixel() const
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected scanline pixel"};
    }
    return {m_col, m_row};
}

void ScanlinePass::suspend()
{
    if (!m_standard_calc_active)
    {
        return;
    }
    if (m_current_pass == 1)
    {
        add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, m_col, m_row, 0, g_work_symmetry);
    }
    else
    {
        add_work_list(
            g_start_pt.x, m_row, g_stop_pt.x, stop_row_for_resume(), m_col, m_row, g_work_pass, g_work_symmetry);
    }
}

bool ScanlinePass::select_current_pass_pixel()
{
    g_row = m_row;
    g_col = m_col;

    while (m_row <= g_i_stop_pt.y)
    {
        g_current_row = m_row;
        while (m_col <= g_i_stop_pt.x)
        {
            g_row = m_row;
            g_col = m_col;
            // on 2nd pass of two, skip even pts
            if (g_quick_calc && !g_resuming)
            {
                g_color = get_color(m_col, m_row);
                if (g_color != g_inside_color)
                {
                    ++m_col;
                    continue;
                }
            }
            if (m_current_pass == 1 || m_calc_mode == CalcMode::ONE_PASS || (m_row & 1) != 0 || (m_col & 1) != 0)
            {
                m_pixel_selected = true;
                return true;
            }
            ++m_col;
        }
        m_col = g_i_start_pt.x;
        if (m_current_pass == 1 && (m_row & 1) == 0)
        {
            ++m_row;
        }
        ++m_row;
        if (m_row <= g_i_stop_pt.y)
        {
            g_reset_periodicity = true;
        }
    }
    g_row = m_row;
    g_col = m_col;
    m_standard_calc_active = false;
    return false;
}

void ScanlinePass::complete_pixel(const int color)
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected scanline pixel to complete"};
    }
    g_row = m_row;
    g_col = m_col;
    g_color = color;
    g_resuming = false;      // reset so quick_calc works
    g_reset_periodicity = false;
    if (m_current_pass == 1) // first pass, copy pixel and bump col
    {
        if ((m_row & 1) == 0 && m_row < g_i_stop_pt.y)
        {
            g_plot(m_col, m_row + 1, color);
            if ((m_col & 1) == 0 && m_col < g_i_stop_pt.x)
            {
                g_plot(m_col + 1, m_row + 1, color);
            }
        }
        if ((m_col & 1) == 0 && m_col < g_i_stop_pt.x)
        {
            ++m_col;
            g_col = m_col;
            g_plot(m_col, m_row, color);
        }
    }
    ++m_col;
    m_pixel_selected = false;
}

// TODO: removed later
// Slice 26.8 moves calculation to the common StandardFractal boundary.
bool ScanlinePass::iterate()
{
    if (m_standard_calc_active)
    {
        g_reset_periodicity = true;
    }
    while (select_pixel())
    {
        const int result{calc_type()};
        m_row = g_row;
        m_col = g_col;
        if (result == -1)
        {
            if (StandardFractal *standard_fractal = active_standard_fractal(); standard_fractal != nullptr)
            {
                static_cast<void>(standard_fractal->consume_traversal_yield());
            }
            return false;
        }
        complete_pixel(g_color);
        if (StandardFractal *standard_fractal = active_standard_fractal(); g_dispatch.calc_type() == lyapunov_type ||
            (standard_fractal != nullptr && standard_fractal->consume_traversal_yield()))
        {
            return false;
        }
    }
    return true;
}

int ScanlinePass::stop_row_for_resume() const
{
    int stop_row = g_stop_pt.y;
    if (g_i_stop_pt.y != g_stop_pt.y) // must be due to symmetry
    {
        stop_row -= m_row - g_i_start_pt.y;
    }
    return stop_row;
}

} // namespace id::engine
