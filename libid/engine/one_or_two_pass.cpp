// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/one_or_two_pass.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "engine/resume.h"
#include "engine/work_list.h"
#include "ui/video.h"

// routines in this module
static int  standard_calc(int pass_num);

int one_or_two_pass()
{
    g_total_passes = 1;
    if (g_std_calc_mode == '2')
    {
        g_total_passes = 2;
    }
    if (g_std_calc_mode == '2' && g_work_pass == 0) // do 1st pass of two
    {
        if (standard_calc(1) == -1)
        {
            add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_col, g_row, 0, g_work_symmetry);
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
    if (standard_calc(2) == -1)
    {
        int i = g_stop_pt.y;
        if (g_i_stop_pt.y != g_stop_pt.y)   // must be due to symmetry
        {
            i -= g_row - g_i_start_pt.y;
        }
        add_work_list(g_start_pt.x, g_row, g_stop_pt.x, i, g_col, g_row, g_work_pass, g_work_symmetry);
        return -1;
    }

    return 0;
}

static int standard_calc(int pass_num)
{
    g_passes = Passes::SEQUENTIAL_SCAN;
    g_current_pass = pass_num;
    g_row = g_begin_pt.y;
    g_col = g_begin_pt.x;

    while (g_row <= g_i_stop_pt.y)
    {
        g_current_row = g_row;
        g_reset_periodicity = true;
        while (g_col <= g_i_stop_pt.x)
        {
            // on 2nd pass of two, skip even pts
            if (g_quick_calc && !g_resuming)
            {
                g_color = get_color(g_col, g_row);
                if (g_color != g_inside_color)
                {
                    ++g_col;
                    continue;
                }
            }
            if (pass_num == 1 || g_std_calc_mode == '1' || (g_row&1) != 0 || (g_col&1) != 0)
            {
                if (g_calc_type() == -1)   // standard_fractal(), calcmand() or calcmandfp()
                {
                    return -1;          // interrupted
                }
                g_resuming = false;       // reset so quick_calc works
                g_reset_periodicity = false;
                if (pass_num == 1)       // first pass, copy pixel and bump col
                {
                    if ((g_row&1) == 0 && g_row < g_i_stop_pt.y)
                    {
                        g_plot(g_col, g_row+1, g_color);
                        if ((g_col&1) == 0 && g_col < g_i_stop_pt.x)
                        {
                            g_plot(g_col+1, g_row+1, g_color);
                        }
                    }
                    if ((g_col&1) == 0 && g_col < g_i_stop_pt.x)
                    {
                        g_plot(++g_col, g_row, g_color);
                    }
                }
            }
            ++g_col;
        }
        g_col = g_i_start_pt.x;
        if (pass_num == 1 && (g_row&1) == 0)
        {
            ++g_row;
        }
        ++g_row;
    }
    return 0;
}
