// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/tesseral.h"

#include "engine/calcfrac.h"
#include "engine/check_key.h"
#include "engine/id_data.h"
#include "engine/pixel_limits.h"
#include "engine/work_list.h"
#include "ui/cmdfiles.h"
#include "ui/video.h"

#include <cstring>

static int tess_check_col(int x, int y1, int y2);
static int tess_check_row(int x1, int x2, int y);
static int tess_col(int x, int y1, int y2);
static int tess_row(int x1, int x2, int y);

static bool s_guess_plot{};  // paint 1st pass row at a time?
static Byte s_stack[4096]{}; // common temp, two put_line calls

// tesseral method by CJLT begins here

struct Tess             // one of these per box to be done gets stacked
{
    int x1, x2, y1, y2;      // left/right top/bottom x/y coords
    int top, bot, lft, rgt;  // edge colors, -1 mixed, -2 unknown
};

int tesseral()
{
    s_guess_plot = (g_plot != g_put_color && g_plot != sym_plot2);
    Tess *tp = (Tess *) &s_stack[0];
    tp->x1 = g_i_x_start;                              // set up initial box
    tp->x2 = g_i_x_stop;
    tp->y1 = g_i_y_start;
    tp->y2 = g_i_y_stop;

    if (g_work_pass == 0) // not resuming
    {
        tp->top = tess_row(g_i_x_start, g_i_x_stop, g_i_y_start);     // Do top row
        tp->bot = tess_row(g_i_x_start, g_i_x_stop, g_i_y_stop);      // Do bottom row
        tp->lft = tess_col(g_i_x_start, g_i_y_start+1, g_i_y_stop-1); // Do left column
        tp->rgt = tess_col(g_i_x_stop, g_i_y_start+1, g_i_y_stop-1);  // Do right column
        if (check_key())
        {
            // interrupt before we got properly rolling
            add_work_list(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 0, g_work_symmetry);
            return -1;
        }
    }
    else // resuming, rebuild work stack
    {
        int mid;
        tp->rgt = -2;
        tp->lft = -2;
        tp->bot = -2;
        tp->top = -2;
        int cur_y = g_yy_begin & 0xfff;
        int y_size = 1;
        int i = (unsigned) g_yy_begin >> 12;
        while (--i >= 0)
        {
            y_size <<= 1;
        }
        int cur_x = g_work_pass & 0xfff;
        int x_size = 1;
        i = (unsigned)g_work_pass >> 12;
        while (--i >= 0)
        {
            x_size <<= 1;
        }
        while (true)
        {
            Tess *tp2 = tp;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // next divide down middle
                if (tp->x1 == cur_x && (tp->x2 - tp->x1 - 2) < x_size)
                {
                    break;
                }
                mid = (tp->x1 + tp->x2) >> 1;                // Find mid-point
                if (mid > cur_x)
                {
                    // stack right part
                    std::memcpy(++tp, tp2, sizeof(*tp));
                    tp->x2 = mid;
                }
                tp2->x1 = mid;
            }
            else
            {
                // next divide across
                if (tp->y1 == cur_y && (tp->y2 - tp->y1 - 2) < y_size)
                {
                    break;
                }
                mid = (tp->y1 + tp->y2) >> 1;                // Find mid-point
                if (mid > cur_y)
                {
                    // stack bottom part
                    std::memcpy(++tp, tp2, sizeof(*tp));
                    tp->y2 = mid;
                }
                tp2->y1 = mid;
            }
        }
    }

    g_got_status = StatusValues::TESSERAL; // for tab_display

    while (tp >= (Tess *)&s_stack[0])
    {
        // do next box
        g_current_column = tp->x1; // for tab_display
        g_current_row = tp->y1;

        if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
        {
            goto tess_split;
        }
        // for any edge whose color is unknown, set it
        if (tp->top == -2)
        {
            tp->top = tess_check_row(tp->x1, tp->x2, tp->y1);
        }
        if (tp->top == -1)
        {
            goto tess_split;
        }
        if (tp->bot == -2)
        {
            tp->bot = tess_check_row(tp->x1, tp->x2, tp->y2);
        }
        if (tp->bot != tp->top)
        {
            goto tess_split;
        }
        if (tp->lft == -2)
        {
            tp->lft = tess_check_col(tp->x1, tp->y1, tp->y2);
        }
        if (tp->lft != tp->top)
        {
            goto tess_split;
        }
        if (tp->rgt == -2)
        {
            tp->rgt = tess_check_col(tp->x2, tp->y1, tp->y2);
        }
        if (tp->rgt != tp->top)
        {
            goto tess_split;
        }

        {
            int mid;
            int mid_color;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // divide down the middle
                mid = (tp->x1 + tp->x2) >> 1;           // Find mid-point
                mid_color = tess_col(mid, tp->y1+1, tp->y2-1); // Do mid column
                if (mid_color != tp->top)
                {
                    goto tess_split;
                }
            }
            else
            {
                // divide across the middle
                mid = (tp->y1 + tp->y2) >> 1;           // Find mid-point
                mid_color = tess_row(tp->x1+1, tp->x2-1, mid); // Do mid row
                if (mid_color != tp->top)
                {
                    goto tess_split;
                }
            }
        }

        {
            // all 4 edges are the same color, fill in
            int j;
            if (g_fill_color != 0)
            {
                int i = 0;
                if (g_fill_color > 0)
                {
                    tp->top = g_fill_color %g_colors;
                }
                if (s_guess_plot || (j = tp->x2 - tp->x1 - 1) < 2)
                {
                    // paint dots
                    for (g_col = tp->x1 + 1; g_col < tp->x2; g_col++)
                    {
                        for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
                        {
                            (*g_plot)(g_col, g_row, tp->top);
                            if (++i > 500)
                            {
                                if (check_key())
                                {
                                    goto tess_end;
                                }
                                i = 0;
                            }
                        }
                    }
                }
                else
                {
                    // use put_line for speed
                    std::memset(&s_stack[OLD_MAX_PIXELS], tp->top, j);
                    for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
                    {
                        write_span(g_row, tp->x1+1, tp->x2-1, &s_stack[OLD_MAX_PIXELS]);
                        if (g_plot != g_put_color) // symmetry
                        {
                            j = g_yy_stop-(g_row-g_yy_start);
                            if (j > g_i_y_stop && j < g_logical_screen_y_dots)
                            {
                                write_span(j, tp->x1+1, tp->x2-1, &s_stack[OLD_MAX_PIXELS]);
                            }
                        }
                        if (++i > 25)
                        {
                            if (check_key())
                            {
                                goto tess_end;
                            }
                            i = 0;
                        }
                    }
                }
            }
            --tp;
        }
        continue;

tess_split:
        {
            // box not surrounded by same color, subdivide
            int mid;
            int mid_color;
            Tess *tp2;
            if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
            {
                // divide down the middle
                mid = (tp->x1 + tp->x2) >> 1;                // Find mid-point
                mid_color = tess_col(mid, tp->y1+1, tp->y2-1); // Do mid column
                if (mid_color == -3)
                {
                    goto tess_end;
                }
                if (tp->x2 - mid > 1)
                {
                    // right part >= 1 column
                    if (tp->top == -1)
                    {
                        tp->top = -2;
                    }
                    if (tp->bot == -1)
                    {
                        tp->bot = -2;
                    }
                    tp2 = tp;
                    if (mid - tp->x1 > 1)
                    {
                        // left part >= 1 col, stack right
                        std::memcpy(++tp, tp2, sizeof(*tp));
                        tp->x2 = mid;
                        tp->rgt = mid_color;
                    }
                    tp2->x1 = mid;
                    tp2->lft = mid_color;
                }
                else
                {
                    --tp;
                }
            }
            else
            {
                // divide across the middle
                mid = (tp->y1 + tp->y2) >> 1;                // Find mid-point
                mid_color = tess_row(tp->x1+1, tp->x2-1, mid); // Do mid row
                if (mid_color == -3)
                {
                    goto tess_end;
                }
                if (tp->y2 - mid > 1)
                {
                    // bottom part >= 1 column
                    if (tp->lft == -1)
                    {
                        tp->lft = -2;
                    }
                    if (tp->rgt == -1)
                    {
                        tp->rgt = -2;
                    }
                    tp2 = tp;
                    if (mid - tp->y1 > 1)
                    {
                        // top also >= 1 col, stack bottom
                        std::memcpy(++tp, tp2, sizeof(*tp));
                        tp->y2 = mid;
                        tp->bot = mid_color;
                    }
                    tp2->y1 = mid;
                    tp2->top = mid_color;
                }
                else
                {
                    --tp;
                }
            }
        }
    }

tess_end:
    if (tp >= (Tess *)&s_stack[0])
    {
        // didn't complete
        int y_size = 1;
        int x_size = 1;
        int i = 2;
        while (tp->x2 - tp->x1 - 2 >= i)
        {
            i <<= 1;
            ++x_size;
        }
        i = 2;
        while (tp->y2 - tp->y1 - 2 >= i)
        {
            i <<= 1;
            ++y_size;
        }
        add_work_list(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
                     (y_size << 12)+tp->y1, (x_size << 12)+tp->x1, g_work_symmetry);
        return -1;
    }
    return 0;
} // tesseral

static int tess_check_col(int x, int y1, int y2)
{
    int i = get_color(x, ++y1);
    while (--y2 > y1)
    {
        if (get_color(x, y2) != i)
        {
            return -1;
        }
    }
    return i;
}

static int tess_check_row(int x1, int x2, int y)
{
    int i = get_color(x1, y);
    while (x2 > x1)
    {
        if (get_color(x2, y) != i)
        {
            return -1;
        }
        --x2;
    }
    return i;
}

static int tess_col(int x, int y1, int y2)
{
    g_col = x;
    g_row = y1;
    g_reset_periodicity = true;
    int col_color = (*g_calc_type)();
    // cppcheck-suppress redundantAssignment
    g_reset_periodicity = false;
    while (++g_row <= y2)
    {
        // generate the column
        int i = (*g_calc_type)();
        if (i < 0)
        {
            return -3;
        }
        if (i != col_color)
        {
            col_color = -1;
        }
    }
    return col_color;
}

static int tess_row(int x1, int x2, int y)
{
    g_row = y;
    g_col = x1;
    g_reset_periodicity = true;
    int row_color = (*g_calc_type)();
    // cppcheck-suppress redundantAssignment
    g_reset_periodicity = false;
    while (++g_col <= x2)
    {
        // generate the row
        int i = (*g_calc_type)();
        if (i < 0)
        {
            return -3;
        }
        if (i != row_color)
        {
            row_color = -1;
        }
    }
    return row_color;
}
