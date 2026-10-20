// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/ScanlinePass.h"

#include "engine/calcfrac.h"
#include "engine/resume.h"
#include "engine/StandardFractal.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/lyapunov.h"
#include "ui/video.h"

using namespace id::ui;
using namespace id::fractals;

namespace id::engine
{

bool OnePass::iterate()
{
    g_total_passes = 1;
    return standard_calc(2, CalcMode::ONE_PASS);
}

bool TwoPass::iterate()
{
    g_total_passes = 2;
    if (g_work_pass == 0) // do 1st pass of two
    {
        if (!standard_calc(1, CalcMode::TWO_PASS))
        {
            return false;
        }
        if (g_num_work_list > 0) // worklist not empty, defer 2nd pass
        {
            add_work_list(
                g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x, g_start_pt.y, 1, g_work_symmetry);
            return true;
        }
        g_work_pass = 1;
        g_begin_pt.x = g_start_pt.x;
        g_begin_pt.y = g_start_pt.y;
    }
    // second or only pass
    return standard_calc(2, CalcMode::TWO_PASS);
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

bool ScanlinePass::standard_calc(const int pass_num, const CalcMode calc_mode)
{
    g_passes = Passes::SEQUENTIAL_SCAN;
    m_current_pass = pass_num;
    g_current_pass = m_current_pass;
    if (!m_standard_calc_active)
    {
        m_row = g_begin_pt.y;
        m_col = g_begin_pt.x;
        m_standard_calc_active = true;
    }
    g_row = m_row;
    g_col = m_col;

    while (m_row <= g_i_stop_pt.y)
    {
        g_current_row = m_row;
        g_reset_periodicity = true;
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
            if (pass_num == 1 || calc_mode == CalcMode::ONE_PASS || (m_row & 1) != 0 || (m_col & 1) != 0)
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
                g_resuming = false; // reset so quick_calc works
                g_reset_periodicity = false;
                if (pass_num == 1)  // first pass, copy pixel and bump col
                {
                    if ((m_row & 1) == 0 && m_row < g_i_stop_pt.y)
                    {
                        g_plot(m_col, m_row + 1, g_color);
                        if ((m_col & 1) == 0 && m_col < g_i_stop_pt.x)
                        {
                            g_plot(m_col + 1, m_row + 1, g_color);
                        }
                    }
                    if ((m_col & 1) == 0 && m_col < g_i_stop_pt.x)
                    {
                        ++m_col;
                        g_col = m_col;
                        g_plot(m_col, m_row, g_color);
                    }
                }
            }
            ++m_col;
            if (StandardFractal *standard_fractal = active_standard_fractal();
                g_dispatch.calc_type() == lyapunov_type ||
                (standard_fractal != nullptr && standard_fractal->consume_traversal_yield()))
            {
                return false;
            }
        }
        m_col = g_i_start_pt.x;
        if (pass_num == 1 && (m_row & 1) == 0)
        {
            ++m_row;
        }
        ++m_row;
    }
    g_row = m_row;
    g_col = m_col;
    m_standard_calc_active = false;
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
