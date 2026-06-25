// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/one_or_two_pass.h"

#include "engine/calcfrac.h"
#include "engine/resume.h"
#include "engine/StandardFractal.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "ui/KeyboardHandler.h"
#include "ui/video.h"

using namespace id::ui;
using namespace id::fractals;

namespace id::engine
{

bool OnePass::iterate()
{
    return run() != -1;
}

int OnePass::run()
{
    g_total_passes = 1;
    if (standard_calc(2, CalcMode::ONE_PASS) == -1)
    {
        if (calc_interrupted())
        {
            add_work_list(g_start_pt.x, m_resume_row, g_stop_pt.x, stop_row_for_resume(), m_resume_col, m_resume_row,
                g_work_pass, g_work_symmetry);
        }
        return -1;
    }
    return 0;
}

bool TwoPass::iterate()
{
    return run() != -1;
}

int TwoPass::run()
{
    g_total_passes = 2;
    if (g_work_pass == 0) // do 1st pass of two
    {
        if (standard_calc(1, CalcMode::TWO_PASS) == -1)
        {
            if (calc_interrupted())
            {
                add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, m_resume_col, m_resume_row, 0,
                    g_work_symmetry);
            }
            return -1;
        }
        if (g_num_work_list > 0) // worklist not empty, defer 2nd pass
        {
            add_work_list(
                g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x, g_start_pt.y, 1, g_work_symmetry);
            return 0;
        }
        g_work_pass = 1;
        g_begin_pt.x = g_start_pt.x;
        g_begin_pt.y = g_start_pt.y;
    }
    // second or only pass
    if (standard_calc(2, CalcMode::TWO_PASS) == -1)
    {
        if (calc_interrupted())
        {
            add_work_list(g_start_pt.x, m_resume_row, g_stop_pt.x, stop_row_for_resume(), m_resume_col, m_resume_row,
                g_work_pass, g_work_symmetry);
        }
        return -1;
    }

    return 0;
}

int OneOrTwoPass::standard_calc(const int pass_num, const CalcMode calc_mode)
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
                const int result = calc_type(); // standard_fractal(), calcmand() or calcmandfp()
                m_row = g_row;
                m_col = g_col;
                if (result == -1)
                {
                    m_resume_row = m_row;
                    m_resume_col = m_col;
                    return -1;      // interrupted
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
                standard_fractal != nullptr && standard_fractal->consume_standard_pixel_yield())
            {
                return -1;
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
    return 0;
}

int OneOrTwoPass::stop_row_for_resume() const
{
    int stop_row = g_stop_pt.y;
    if (g_i_stop_pt.y != g_stop_pt.y) // must be due to symmetry
    {
        stop_row -= m_resume_row - g_i_start_pt.y;
    }
    return stop_row;
}

int one_or_two_pass()
{
    if (g_std_calc_mode == CalcMode::TWO_PASS)
    {
        TwoPass pass;
        return pass.run();
    }

    OnePass pass;
    return pass.run();
}

} // namespace id::engine
