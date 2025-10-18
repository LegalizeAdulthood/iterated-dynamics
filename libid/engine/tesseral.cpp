// SPDX-License-Identifier: GPL-3.0-only
//
// tesseral pass algorithm by Chris Taylor

#include "engine/tesseral.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/LogicalScreen.h"
#include "engine/work_list.h"
#include "ui/check_key.h"
#include "ui/video.h"

#include <vector>

using namespace id::ui;

namespace id::engine
{

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

class Tesseral
{
public:
    Tesseral();

    bool done() const;
    void next_box();
    bool split_needed();
    void fill_box();
    bool split_box();
    void suspend();

private:
    bool m_guess_plot{};  // paint 1st pass row at a time?
    Tess m_stack[4096 / sizeof(Tess)]{};
    Tess *m_tp{};
};

static int tess_check_col(int x, int y1, int y2);
static int tess_check_row(int x1, int x2, int y);
static int tess_col(int x, int y1, int y2);
static int tess_row(int x1, int x2, int y);

Tesseral::Tesseral() :
    m_guess_plot(g_plot != g_put_color && g_plot != sym_plot2),
    m_tp(&m_stack[0])
{
    m_tp->x1 = g_i_start_pt.x;                              // set up initial box
    m_tp->x2 = g_i_stop_pt.x;
    m_tp->y1 = g_i_start_pt.y;
    m_tp->y2 = g_i_stop_pt.y;

    if (g_work_pass == 0) // not resuming
    {
        m_tp->top = tess_row(g_i_start_pt.x, g_i_stop_pt.x, g_i_start_pt.y);     // Do top row
        m_tp->bot = tess_row(g_i_start_pt.x, g_i_stop_pt.x, g_i_stop_pt.y);      // Do bottom row
        m_tp->lft = tess_col(g_i_start_pt.x, g_i_start_pt.y+1, g_i_stop_pt.y-1); // Do left column
        m_tp->rgt = tess_col(g_i_stop_pt.x, g_i_start_pt.y+1, g_i_stop_pt.y-1);  // Do right column
    }
    else // resuming, rebuild work stack
    {
        m_tp->rgt = -2;
        m_tp->lft = -2;
        m_tp->bot = -2;
        m_tp->top = -2;
        const int cur_y = g_begin_pt.y & 0xfff;
        int y_size = 1;
        int i = static_cast<unsigned>(g_begin_pt.y) >> 12;
        while (--i >= 0)
        {
            y_size <<= 1;
        }
        const int cur_x = g_work_pass & 0xfff;
        int x_size = 1;
        i = static_cast<unsigned>(g_work_pass) >> 12;
        while (--i >= 0)
        {
            x_size <<= 1;
        }
        while (true)
        {
            Tess *tp2 = m_tp;
            if (m_tp->x2 - m_tp->x1 > m_tp->y2 - m_tp->y1)
            {
                // next divide down middle
                if (m_tp->x1 == cur_x && m_tp->x2 - m_tp->x1 - 2 < x_size)
                {
                    break;
                }
                const int mid = (m_tp->x1 + m_tp->x2) >> 1;                // Find mid-point
                if (mid > cur_x)
                {
                    // stack right part
                    *++m_tp = *tp2; // copy current box
                    m_tp->x2 = mid;
                }
                tp2->x1 = mid;
            }
            else
            {
                // next divide across
                if (m_tp->y1 == cur_y && m_tp->y2 - m_tp->y1 - 2 < y_size)
                {
                    break;
                }
                const int mid = (m_tp->y1 + m_tp->y2) >> 1;                // Find mid-point
                if (mid > cur_y)
                {
                    // stack bottom part
                    *++m_tp = *tp2; // copy current box
                    m_tp->y2 = mid;
                }
                tp2->y1 = mid;
            }
        }
    }

    g_passes = Passes::TESSERAL; // for tab_display
}

bool Tesseral::done() const
{
    return m_tp < &m_stack[0];
}

void Tesseral::next_box()
{
    // do next box
    g_current_column = m_tp->x1; // for tab_display
    g_current_row = m_tp->y1;
}

bool Tesseral::split_needed()
{
    if (m_tp->top == -1 || m_tp->bot == -1 || m_tp->lft == -1 || m_tp->rgt == -1)
    {
        return true;
    }
    // for any edge whose color is unknown, set it
    if (m_tp->top == -2)
    {
        m_tp->top = tess_check_row(m_tp->x1, m_tp->x2, m_tp->y1);
    }
    if (m_tp->top == -1)
    {
        return true;
    }
    if (m_tp->bot == -2)
    {
        m_tp->bot = tess_check_row(m_tp->x1, m_tp->x2, m_tp->y2);
    }
    if (m_tp->bot != m_tp->top)
    {
        return true;
    }
    if (m_tp->lft == -2)
    {
        m_tp->lft = tess_check_col(m_tp->x1, m_tp->y1, m_tp->y2);
    }
    if (m_tp->lft != m_tp->top)
    {
        return true;
    }
    if (m_tp->rgt == -2)
    {
        m_tp->rgt = tess_check_col(m_tp->x2, m_tp->y1, m_tp->y2);
    }
    if (m_tp->rgt != m_tp->top)
    {
        return true;
    }

    if (m_tp->x2 - m_tp->x1 > m_tp->y2 - m_tp->y1)
    {
        // divide down the middle
        const int mid = (m_tp->x1 + m_tp->x2) >> 1;                      // Find mid-point
        const int mid_color = tess_col(mid, m_tp->y1 + 1, m_tp->y2 - 1); // Do mid column
        if (mid_color != m_tp->top)
        {
            return true;
        }
    }
    else
    {
        // divide across the middle
        const int mid = (m_tp->y1 + m_tp->y2) >> 1;                      // Find mid-point
        const int mid_color = tess_row(m_tp->x1 + 1, m_tp->x2 - 1, mid); // Do mid row
        if (mid_color != m_tp->top)
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
            m_tp->top = g_fill_color % g_colors;
        }
        if (const int j = m_tp->x2 - m_tp->x1 - 1; m_guess_plot || j < 2)
        {
            for (g_col = m_tp->x1 + 1; g_col < m_tp->x2; g_col++)
            {
                for (g_row = m_tp->y1 + 1; g_row < m_tp->y2; g_row++)
                {
                    g_plot(g_col, g_row, m_tp->top);
                }
            }
        }
        else
        {
            // use write_span for speed
            const std::vector pixels(j, static_cast<Byte>(m_tp->top));
            for (g_row = m_tp->y1 + 1; g_row < m_tp->y2; g_row++)
            {
                write_span(g_row, m_tp->x1 + 1, m_tp->x2 - 1, pixels.data());
                if (g_plot != g_put_color) // symmetry
                {
                    if (const int k = g_stop_pt.y - (g_row - g_start_pt.y);
                        k > g_i_stop_pt.y && k < g_logical_screen.y_dots)
                    {
                        write_span(k, m_tp->x1 + 1, m_tp->x2 - 1, pixels.data());
                    }
                }
            }
        }
    }
    --m_tp;
}

bool Tesseral::split_box()
{
    // box not surrounded by same color, subdivide
    int mid;
    int mid_color;
    Tess *tp2;
    if (m_tp->x2 - m_tp->x1 > m_tp->y2 - m_tp->y1)
    {
        // divide down the middle
        mid = (m_tp->x1 + m_tp->x2) >> 1;                      // Find mid-point
        mid_color = tess_col(mid, m_tp->y1 + 1, m_tp->y2 - 1); // Do mid column
        if (mid_color == -3)
        {
            return true;
        }
        if (m_tp->x2 - mid > 1)
        {
            // right part >= 1 column
            if (m_tp->top == -1)
            {
                m_tp->top = -2;
            }
            if (m_tp->bot == -1)
            {
                m_tp->bot = -2;
            }
            tp2 = m_tp;
            if (mid - m_tp->x1 > 1)
            {
                // left part >= 1 col, stack right
                *++m_tp = *tp2; // copy current box
                m_tp->x2 = mid;
                m_tp->rgt = mid_color;
            }
            tp2->x1 = mid;
            tp2->lft = mid_color;
        }
        else
        {
            --m_tp;
        }
    }
    else
    {
        // divide across the middle
        mid = (m_tp->y1 + m_tp->y2) >> 1;                      // Find mid-point
        mid_color = tess_row(m_tp->x1 + 1, m_tp->x2 - 1, mid); // Do mid row
        if (mid_color == -3)
        {
            return true;
        }
        if (m_tp->y2 - mid > 1)
        {
            // bottom part >= 1 column
            if (m_tp->lft == -1)
            {
                m_tp->lft = -2;
            }
            if (m_tp->rgt == -1)
            {
                m_tp->rgt = -2;
            }
            tp2 = m_tp;
            if (mid - m_tp->y1 > 1)
            {
                // top also >= 1 col, stack bottom
                *++m_tp = *tp2; // copy current box
                m_tp->y2 = mid;
                m_tp->bot = mid_color;
            }
            tp2->y1 = mid;
            tp2->top = mid_color;
        }
        else
        {
            --m_tp;
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
    while (m_tp->x2 - m_tp->x1 - 2 >= i)
    {
        i <<= 1;
        ++x_size;
    }
    i = 2;
    while (m_tp->y2 - m_tp->y1 - 2 >= i)
    {
        i <<= 1;
        ++y_size;
    }
    add_work_list(g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x,
        (y_size << 12) + m_tp->y1, (x_size << 12) + m_tp->x1, g_work_symmetry);
}

int tesseral()
{
    Tesseral tess;

    while (!tess.done())
    {
        tess.next_box();

        if (tess.split_needed())
        {
            if (tess.split_box())
            {
                break;
            }
        }
        else
        {
            tess.fill_box();
            if (check_key())
            {
                break;
            }
        }
    }

    if (!tess.done())
    {
        tess.suspend();
        return -1;
    }

    return 0;
}

static int tess_check_col(const int x, int y1, int y2)
{
    const int i = get_color(x, ++y1);
    while (--y2 > y1)
    {
        if (get_color(x, y2) != i)
        {
            return -1;
        }
    }
    return i;
}

static int tess_check_row(const int x1, int x2, const int y)
{
    const int i = get_color(x1, y);
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

static int tess_col(const int x, const int y1, const int y2)
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
        const int i = g_calc_type();
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

static int tess_row(const int x1, const int x2, const int y)
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
        const int i = g_calc_type();
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

} // namespace id::engine
