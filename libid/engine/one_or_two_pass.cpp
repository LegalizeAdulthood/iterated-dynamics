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
            add_work_list(g_xx_start, g_xx_stop, g_col, g_yy_start, g_yy_stop, g_row, 0, g_work_symmetry);
            return -1;
        }
        if (g_num_work_list > 0) // worklist not empty, defer 2nd pass
        {
            add_work_list(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 1, g_work_symmetry);
            return 0;
        }
        g_work_pass = 1;
        g_xx_begin = g_xx_start;
        g_yy_begin = g_yy_start;
    }
    // second or only pass
    if (standard_calc(2) == -1)
    {
        int i = g_yy_stop;
        if (g_i_y_stop != g_yy_stop)   // must be due to symmetry
        {
            i -= g_row - g_i_y_start;
        }
        add_work_list(g_xx_start, g_xx_stop, g_col, g_row, i, g_row, g_work_pass, g_work_symmetry);
        return -1;
    }

    return 0;
}

static int standard_calc(int pass_num)
{
    g_got_status = StatusValues::ONE_OR_TWO_PASS;
    g_current_pass = pass_num;
    g_row = g_yy_begin;
    g_col = g_xx_begin;

    while (g_row <= g_i_y_stop)
    {
        g_current_row = g_row;
        g_reset_periodicity = true;
        while (g_col <= g_i_x_stop)
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
                if ((*g_calc_type)() == -1)   // standard_fractal(), calcmand() or calcmandfp()
                {
                    return -1;          // interrupted
                }
                g_resuming = false;       // reset so quick_calc works
                g_reset_periodicity = false;
                if (pass_num == 1)       // first pass, copy pixel and bump col
                {
                    if ((g_row&1) == 0 && g_row < g_i_y_stop)
                    {
                        (*g_plot)(g_col, g_row+1, g_color);
                        if ((g_col&1) == 0 && g_col < g_i_x_stop)
                        {
                            (*g_plot)(g_col+1, g_row+1, g_color);
                        }
                    }
                    if ((g_col&1) == 0 && g_col < g_i_x_stop)
                    {
                        (*g_plot)(++g_col, g_row, g_color);
                    }
                }
            }
            ++g_col;
        }
        g_col = g_i_x_start;
        if (pass_num == 1 && (g_row&1) == 0)
        {
            ++g_row;
        }
        ++g_row;
    }
    return 0;
}
