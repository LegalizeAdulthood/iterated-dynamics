// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/tesseral.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "engine/work_list.h"
#include "ui/check_key.h"
#include "engine/cmdfiles.h"
#include "ui/video.h"

#include <vector>

struct Tess             // one of these per box to be done gets stacked
{
    int x1{};
    int x2{};
    int y1{};
    int y2{};                  // left/right top/bottom x/y coords
    int top{};
    int bot{};
    int lft{};
    int rgt{}; // edge colors, -1 mixed, -2 unknown
};

struct Tesseral
{
    void init();
    bool more() const;

    bool guess_plot{};  // paint 1st pass row at a time?
    Tess stack[4096 / sizeof(Tess)]{};
    Tess *tp{};
};

static int tess_check_col(int x, int y1, int y2);
static int tess_check_row(int x1, int x2, int y);
static int tess_col(int x, int y1, int y2);
static int tess_row(int x1, int x2, int y);

static Tesseral s_tess;

void Tesseral::init()
{
    s_tess.guess_plot = (g_plot != g_put_color && g_plot != sym_plot2);
    tp = &s_tess.stack[0];
    tp->x1 = g_i_start_pt.x;                              // set up initial box
    tp->x2 = g_i_stop_pt.x;
    tp->y1 = g_i_start_pt.y;
    tp->y2 = g_i_stop_pt.y;

    if (g_work_pass == 0) // not resuming
    {
        tp->top = tess_row(g_i_start_pt.x, g_i_stop_pt.x, g_i_start_pt.y);     // Do top row
        tp->bot = tess_row(g_i_start_pt.x, g_i_stop_pt.x, g_i_stop_pt.y);      // Do bottom row
        tp->lft = tess_col(g_i_start_pt.x, g_i_start_pt.y+1, g_i_stop_pt.y-1); // Do left column
        tp->rgt = tess_col(g_i_stop_pt.x, g_i_start_pt.y+1, g_i_stop_pt.y-1);  // Do right column
    }
    else // resuming, rebuild work stack
    {
        int mid;
        tp->rgt = -2;
        tp->lft = -2;
        tp->bot = -2;
        tp->top = -2;
        int cur_y = g_begin_pt.y & 0xfff;
        int y_size = 1;
        int i = (unsigned) g_begin_pt.y >> 12;
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
                    *(++tp) = *tp2; // copy current box
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
                    *(++tp) = *tp2; // copy current box
                    tp->y2 = mid;
                }
                tp2->y1 = mid;
            }
        }
    }

    g_got_status = StatusValues::TESSERAL; // for tab_display
}

bool Tesseral::more() const
{
    return tp >= &s_tess.stack[0];
}

// tesseral method by CJLT begins here
int tesseral()
{
    s_tess.init();

    while (s_tess.more())
    {
        // do next box
        g_current_column = s_tess.tp->x1; // for tab_display
        g_current_row = s_tess.tp->y1;

        if (s_tess.tp->top == -1 || s_tess.tp->bot == -1 || s_tess.tp->lft == -1 || s_tess.tp->rgt == -1)
        {
            goto tess_split;
        }
        // for any edge whose color is unknown, set it
        if (s_tess.tp->top == -2)
        {
            s_tess.tp->top = tess_check_row(s_tess.tp->x1, s_tess.tp->x2, s_tess.tp->y1);
        }
        if (s_tess.tp->top == -1)
        {
            goto tess_split;
        }
        if (s_tess.tp->bot == -2)
        {
            s_tess.tp->bot = tess_check_row(s_tess.tp->x1, s_tess.tp->x2, s_tess.tp->y2);
        }
        if (s_tess.tp->bot != s_tess.tp->top)
        {
            goto tess_split;
        }
        if (s_tess.tp->lft == -2)
        {
            s_tess.tp->lft = tess_check_col(s_tess.tp->x1, s_tess.tp->y1, s_tess.tp->y2);
        }
        if (s_tess.tp->lft != s_tess.tp->top)
        {
            goto tess_split;
        }
        if (s_tess.tp->rgt == -2)
        {
            s_tess.tp->rgt = tess_check_col(s_tess.tp->x2, s_tess.tp->y1, s_tess.tp->y2);
        }
        if (s_tess.tp->rgt != s_tess.tp->top)
        {
            goto tess_split;
        }

        {
            int mid;
            int mid_color;
            if (s_tess.tp->x2 - s_tess.tp->x1 > s_tess.tp->y2 - s_tess.tp->y1)
            {
                // divide down the middle
                mid = (s_tess.tp->x1 + s_tess.tp->x2) >> 1;           // Find mid-point
                mid_color = tess_col(mid, s_tess.tp->y1+1, s_tess.tp->y2-1); // Do mid column
                if (mid_color != s_tess.tp->top)
                {
                    goto tess_split;
                }
            }
            else
            {
                // divide across the middle
                mid = (s_tess.tp->y1 + s_tess.tp->y2) >> 1;           // Find mid-point
                mid_color = tess_row(s_tess.tp->x1+1, s_tess.tp->x2-1, mid); // Do mid row
                if (mid_color != s_tess.tp->top)
                {
                    goto tess_split;
                }
            }
        }

        {
            // all 4 edges are the same color, fill in
            if (g_fill_color != 0)
            {
                int i = 0;
                if (g_fill_color > 0)
                {
                    s_tess.tp->top = g_fill_color %g_colors;
                }
                if (const int j = s_tess.tp->x2 - s_tess.tp->x1 - 1; s_tess.guess_plot || j < 2)
                {
                    // paint dots
                    for (g_col = s_tess.tp->x1 + 1; g_col < s_tess.tp->x2; g_col++)
                    {
                        for (g_row = s_tess.tp->y1 + 1; g_row < s_tess.tp->y2; g_row++)
                        {
                            g_plot(g_col, g_row, s_tess.tp->top);
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
                    std::vector pixels(j, static_cast<Byte>(s_tess.tp->top));
                    for (g_row = s_tess.tp->y1 + 1; g_row < s_tess.tp->y2; g_row++)
                    {
                        write_span(g_row, s_tess.tp->x1+1, s_tess.tp->x2-1, pixels.data());
                        if (g_plot != g_put_color) // symmetry
                        {
                            if (const int k = g_stop_pt.y - (g_row - g_start_pt.y);
                                k > g_i_stop_pt.y && k < g_logical_screen_y_dots)
                            {
                                write_span(k, s_tess.tp->x1+1, s_tess.tp->x2-1, pixels.data());
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
            --s_tess.tp;
        }
        continue;

tess_split:
        {
            // box not surrounded by same color, subdivide
            int mid;
            int mid_color;
            Tess *tp2;
            if (s_tess.tp->x2 - s_tess.tp->x1 > s_tess.tp->y2 - s_tess.tp->y1)
            {
                // divide down the middle
                mid = (s_tess.tp->x1 + s_tess.tp->x2) >> 1;                // Find mid-point
                mid_color = tess_col(mid, s_tess.tp->y1+1, s_tess.tp->y2-1); // Do mid column
                if (mid_color == -3)
                {
                    goto tess_end;
                }
                if (s_tess.tp->x2 - mid > 1)
                {
                    // right part >= 1 column
                    if (s_tess.tp->top == -1)
                    {
                        s_tess.tp->top = -2;
                    }
                    if (s_tess.tp->bot == -1)
                    {
                        s_tess.tp->bot = -2;
                    }
                    tp2 = s_tess.tp;
                    if (mid - s_tess.tp->x1 > 1)
                    {
                        // left part >= 1 col, stack right
                        *(++s_tess.tp) = *tp2; // copy current box
                        s_tess.tp->x2 = mid;
                        s_tess.tp->rgt = mid_color;
                    }
                    tp2->x1 = mid;
                    tp2->lft = mid_color;
                }
                else
                {
                    --s_tess.tp;
                }
            }
            else
            {
                // divide across the middle
                mid = (s_tess.tp->y1 + s_tess.tp->y2) >> 1;                // Find mid-point
                mid_color = tess_row(s_tess.tp->x1+1, s_tess.tp->x2-1, mid); // Do mid row
                if (mid_color == -3)
                {
                    goto tess_end;
                }
                if (s_tess.tp->y2 - mid > 1)
                {
                    // bottom part >= 1 column
                    if (s_tess.tp->lft == -1)
                    {
                        s_tess.tp->lft = -2;
                    }
                    if (s_tess.tp->rgt == -1)
                    {
                        s_tess.tp->rgt = -2;
                    }
                    tp2 = s_tess.tp;
                    if (mid - s_tess.tp->y1 > 1)
                    {
                        // top also >= 1 col, stack bottom
                        *(++s_tess.tp) = *tp2; // copy current box
                        s_tess.tp->y2 = mid;
                        s_tess.tp->bot = mid_color;
                    }
                    tp2->y1 = mid;
                    tp2->top = mid_color;
                }
                else
                {
                    --s_tess.tp;
                }
            }
        }
    }

tess_end:
    if (s_tess.tp >= &s_tess.stack[0])
    {
        // didn't complete
        int y_size = 1;
        int x_size = 1;
        int i = 2;
        while (s_tess.tp->x2 - s_tess.tp->x1 - 2 >= i)
        {
            i <<= 1;
            ++x_size;
        }
        i = 2;
        while (s_tess.tp->y2 - s_tess.tp->y1 - 2 >= i)
        {
            i <<= 1;
            ++y_size;
        }
        add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x, (y_size << 12) + s_tess.tp->y1,
            (x_size << 12) + s_tess.tp->x1, g_work_symmetry);
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
    int col_color = g_calc_type();
    // cppcheck-suppress redundantAssignment
    g_reset_periodicity = false;
    while (++g_row <= y2)
    {
        // generate the column
        int i = g_calc_type();
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
    int row_color = g_calc_type();
    // cppcheck-suppress redundantAssignment
    g_reset_periodicity = false;
    while (++g_col <= x2)
    {
        // generate the row
        int i = g_calc_type();
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
