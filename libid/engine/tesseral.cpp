// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/tesseral.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/work_list.h"
#include "ui/check_key.h"
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
    bool split_needed();
    void fill_box();
    bool split_box();
    void suspend();

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
    guess_plot = (g_plot != g_put_color && g_plot != sym_plot2);
    tp = &stack[0];
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
                const int mid = (tp->x1 + tp->x2) >> 1;                // Find mid-point
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
                const int mid = (tp->y1 + tp->y2) >> 1;                // Find mid-point
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

    g_passes = Passes::TESSERAL; // for tab_display
}

bool Tesseral::more() const
{
    return tp >= &stack[0];
}

bool Tesseral::split_needed()
{
    if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
    {
        return true;
    }
    // for any edge whose color is unknown, set it
    if (tp->top == -2)
    {
        tp->top = tess_check_row(tp->x1, tp->x2, tp->y1);
    }
    if (tp->top == -1)
    {
        return true;
    }
    if (tp->bot == -2)
    {
        tp->bot = tess_check_row(tp->x1, tp->x2, tp->y2);
    }
    if (tp->bot != tp->top)
    {
        return true;
    }
    if (tp->lft == -2)
    {
        tp->lft = tess_check_col(tp->x1, tp->y1, tp->y2);
    }
    if (tp->lft != tp->top)
    {
        return true;
    }
    if (tp->rgt == -2)
    {
        tp->rgt = tess_check_col(tp->x2, tp->y1, tp->y2);
    }
    if (tp->rgt != tp->top)
    {
        return true;
    }

    if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
    {
        // divide down the middle
        const int mid = (tp->x1 + tp->x2) >> 1;                      // Find mid-point
        const int mid_color = tess_col(mid, tp->y1 + 1, tp->y2 - 1); // Do mid column
        if (mid_color != tp->top)
        {
            return true;
        }
    }
    else
    {
        // divide across the middle
        const int mid = (tp->y1 + tp->y2) >> 1;                      // Find mid-point
        const int mid_color = tess_row(tp->x1 + 1, tp->x2 - 1, mid); // Do mid row
        if (mid_color != tp->top)
        {
            return true;
        }
    }

    return false;
}

void Tesseral::fill_box()
{
    // all 4 edges are the same color, fill in
    if (g_fill_color != 0)
    {
        if (g_fill_color > 0)
        {
            tp->top = g_fill_color % g_colors;
        }
        if (const int j = tp->x2 - tp->x1 - 1; guess_plot || j < 2)
        {
            for (g_col = tp->x1 + 1; g_col < tp->x2; g_col++)
            {
                for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
                {
                    g_plot(g_col, g_row, tp->top);
                }
            }
        }
        else
        {
            // use write_span for speed
            std::vector pixels(j, static_cast<Byte>(tp->top));
            for (g_row = tp->y1 + 1; g_row < tp->y2; g_row++)
            {
                write_span(g_row, tp->x1 + 1, tp->x2 - 1, pixels.data());
                if (g_plot != g_put_color) // symmetry
                {
                    if (const int k = g_stop_pt.y - (g_row - g_start_pt.y);
                        k > g_i_stop_pt.y && k < g_logical_screen_y_dots)
                    {
                        write_span(k, tp->x1 + 1, tp->x2 - 1, pixels.data());
                    }
                }
            }
        }
    }
    --tp;
}

bool Tesseral::split_box()
{
    // box not surrounded by same color, subdivide
    int mid;
    int mid_color;
    Tess *tp2;
    if (tp->x2 - tp->x1 > tp->y2 - tp->y1)
    {
        // divide down the middle
        mid = (tp->x1 + tp->x2) >> 1;                      // Find mid-point
        mid_color = tess_col(mid, tp->y1 + 1, tp->y2 - 1); // Do mid column
        if (mid_color == -3)
        {
            return true;
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
                *(++tp) = *tp2; // copy current box
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
        mid = (tp->y1 + tp->y2) >> 1;                      // Find mid-point
        mid_color = tess_row(tp->x1 + 1, tp->x2 - 1, mid); // Do mid row
        if (mid_color == -3)
        {
            return true;
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
                *(++tp) = *tp2; // copy current box
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
    return false;
}

void Tesseral::suspend()
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
    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x,
        (y_size << 12) + tp->y1, (x_size << 12) + tp->x1, g_work_symmetry);
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

        if (s_tess.split_needed())
        {
            if (s_tess.split_box())
            {
                break;
            }
        }
        else
        {
            s_tess.fill_box();
            if (check_key())
            {
                break;
            }
        }
    }

    if (s_tess.tp >= &s_tess.stack[0])
    {
        s_tess.suspend();
        return -1;
    }

    return 0;
}

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
