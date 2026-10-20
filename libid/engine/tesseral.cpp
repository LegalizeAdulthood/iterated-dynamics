// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// tesseral pass algorithm by Chris Taylor

#include "engine/tesseral.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/VideoInfo.h"
#include "engine/work_list.h"
#include "ui/video.h"

#include <stdexcept>
#include <vector>

using namespace id::ui;

namespace id::engine
{

namespace
{

constexpr int TESSERAL_EDGE_CHUNK{512};
constexpr int TESSERAL_BOX_CHUNK{512};
constexpr int TESSERAL_PIXEL_FILL_CHUNK{8192};
constexpr int TESSERAL_SPAN_FILL_CHUNK{64};

} // namespace

static int tess_check_col(int x, int y1, int y2);
static int tess_check_row(int x1, int x2, int y);

Tesseral::Tesseral() :
    m_guess_plot(g_plot != g_put_color && g_plot != sym_plot2),
    m_tp(&m_stack[0])
{
    m_tp->x1 = g_i_start_pt.x; // set up initial box
    m_tp->x2 = g_i_stop_pt.x;
    m_tp->y1 = g_i_start_pt.y;
    m_tp->y2 = g_i_stop_pt.y;

    if (g_work_pass == 0)                                                    // not resuming
    {
        m_init_phase = InitPhase::TOP;
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

bool Tesseral::select_pixel()
{
    if (done())
    {
        return false;
    }
    if (!m_pixel_selected)
    {
        advance();
    }
    if (!m_pixel_selected)
    {
        m_boxes_processed = 0;
        return false;
    }
    const math::Point2i point{selected_pixel()};
    g_col = point.x;
    g_row = point.y;
    if (!m_edge_probe.started)
    {
        g_reset_periodicity = true;
    }
    return true;
}

math::Point2i Tesseral::selected_pixel() const
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected tesseral pixel"};
    }
    return m_edge_probe.kind == EdgeProbeKind::ROW ? math::Point2i{m_edge_probe.current, m_edge_probe.fixed}
                                                   : math::Point2i{m_edge_probe.fixed, m_edge_probe.current};
}

void Tesseral::complete_pixel(const int color)
{
    const math::Point2i point{selected_pixel()};
    g_col = point.x;
    g_row = point.y;
    g_color = color;
    if (!m_edge_probe.started)
    {
        g_reset_periodicity = false;
        m_edge_probe.color = color;
        m_edge_probe.started = true;
    }
    else if (m_edge_probe.color != -1 && color != m_edge_probe.color)
    {
        m_edge_probe.color = -1;
    }
    ++m_edge_probe.current;
    ++m_edge_probe.processed;
    m_pixel_selected = false;
}

void Tesseral::begin_chunk()
{
    m_boxes_processed = 0;
    m_edge_probe.processed = 0;
}

void Tesseral::advance()
{
    if (m_init_phase != InitPhase::COMPLETE)
    {
        initialize_edges();
        return;
    }

    while (m_boxes_processed < TESSERAL_BOX_CHUNK && !done())
    {
        next_box();
        if (m_fill.mode != FillMode::NONE)
        {
            if (!fill_box())
            {
                return;
            }
            ++m_boxes_processed;
            continue;
        }

        if (m_split_box_active)
        {
            if (!split_box())
            {
                return;
            }
            ++m_boxes_processed;
            continue;
        }

        bool split{false};
        if (!split_needed(split))
        {
            return;
        }
        if (split)
        {
            if (!split_box())
            {
                return;
            }
        }
        else
        {
            if (!fill_box())
            {
                return;
            }
        }
        ++m_boxes_processed;
    }
}

bool Tesseral::probe_active() const
{
    return m_edge_probe.kind != EdgeProbeKind::NONE;
}

void Tesseral::reset_edge_probe()
{
    m_edge_probe = {};
}

void Tesseral::start_row_probe(const int x1, const int x2, const int y)
{
    m_edge_probe.kind = EdgeProbeKind::ROW;
    m_edge_probe.fixed = y;
    m_edge_probe.current = x1;
    m_edge_probe.final = x2;
    m_edge_probe.color = 0;
    m_edge_probe.processed = 0;
    m_edge_probe.started = false;
}

void Tesseral::start_column_probe(const int x, const int y1, const int y2)
{
    m_edge_probe.kind = EdgeProbeKind::COLUMN;
    m_edge_probe.fixed = x;
    m_edge_probe.current = y1;
    m_edge_probe.final = y2;
    m_edge_probe.color = 0;
    m_edge_probe.processed = 0;
    m_edge_probe.started = false;
}

bool Tesseral::advance_edge_probe()
{
    if (!probe_active() || (m_edge_probe.started && m_edge_probe.current > m_edge_probe.final))
    {
        return true;
    }
    if (m_edge_probe.processed == TESSERAL_EDGE_CHUNK)
    {
        m_edge_probe.processed = 0;
        return false;
    }
    m_pixel_selected = true;
    return false;
}

bool Tesseral::finish_edge_probe(int &color)
{
    if (!advance_edge_probe())
    {
        return false;
    }

    color = m_edge_probe.color;
    reset_edge_probe();
    return true;
}

bool Tesseral::initialize_edges()
{
    if (probe_active())
    {
        int color;
        if (!finish_edge_probe(color))
        {
            return false;
        }
        switch (m_init_phase)
        {
        case InitPhase::TOP:
            m_tp->top = color;
            m_init_phase = InitPhase::BOTTOM;
            break;

        case InitPhase::BOTTOM:
            m_tp->bot = color;
            m_init_phase = InitPhase::LEFT;
            break;

        case InitPhase::LEFT:
            m_tp->lft = color;
            m_init_phase = InitPhase::RIGHT;
            break;

        case InitPhase::RIGHT:
            m_tp->rgt = color;
            m_init_phase = InitPhase::COMPLETE;
            break;

        case InitPhase::COMPLETE:
        default:
            break;
        }
    }

    switch (m_init_phase)
    {
    case InitPhase::TOP:
        start_row_probe(g_i_start_pt.x, g_i_stop_pt.x, g_i_start_pt.y);
        return initialize_edges();

    case InitPhase::BOTTOM:
        start_row_probe(g_i_start_pt.x, g_i_stop_pt.x, g_i_stop_pt.y);
        return initialize_edges();

    case InitPhase::LEFT:
        start_column_probe(g_i_start_pt.x, g_i_start_pt.y + 1, g_i_stop_pt.y - 1);
        return initialize_edges();

    case InitPhase::RIGHT:
        start_column_probe(g_i_stop_pt.x, g_i_start_pt.y + 1, g_i_stop_pt.y - 1);
        return initialize_edges();

    case InitPhase::COMPLETE:
    default:
        break;
    }

    return true;
}

void Tesseral::next_box()
{
    // do next box
    g_current_column = m_tp->x1; // for tab_display
    g_current_row = m_tp->y1;
}

void Tesseral::reset_split_check()
{
    m_split_check_phase = SplitCheckPhase::START;
}

bool Tesseral::split_needed(bool &needed)
{
    needed = false;
    if (probe_active())
    {
        int color;
        if (!finish_edge_probe(color))
        {
            return false;
        }
        switch (m_split_check_phase)
        {
        case SplitCheckPhase::TOP:
            m_tp->top = color;
            break;

        case SplitCheckPhase::BOTTOM:
            m_tp->bot = color;
            break;

        case SplitCheckPhase::LEFT:
            m_tp->lft = color;
            break;

        case SplitCheckPhase::RIGHT:
            m_tp->rgt = color;
            break;

        case SplitCheckPhase::MIDDLE:
            needed = color != m_tp->top;
            reset_split_check();
            return true;

        case SplitCheckPhase::START:
        default:
            break;
        }
    }

    if (m_split_check_phase == SplitCheckPhase::START &&
        (m_tp->top == -1 || m_tp->bot == -1 || m_tp->lft == -1 || m_tp->rgt == -1))
    {
        needed = true;
        reset_split_check();
        return true;
    }

    // for any edge whose color is unknown, set it
    if (m_split_check_phase == SplitCheckPhase::START)
    {
        m_split_check_phase = SplitCheckPhase::TOP;
    }
    if (m_split_check_phase == SplitCheckPhase::TOP)
    {
        if (m_tp->top == -2)
        {
            m_tp->top = tess_check_row(m_tp->x1, m_tp->x2, m_tp->y1);
        }
        m_split_check_phase = SplitCheckPhase::BOTTOM;
    }
    if (m_tp->top == -1)
    {
        needed = true;
        reset_split_check();
        return true;
    }
    if (m_split_check_phase == SplitCheckPhase::BOTTOM)
    {
        if (m_tp->bot == -2)
        {
            m_tp->bot = tess_check_row(m_tp->x1, m_tp->x2, m_tp->y2);
        }
        m_split_check_phase = SplitCheckPhase::LEFT;
    }
    if (m_tp->bot != m_tp->top)
    {
        needed = true;
        reset_split_check();
        return true;
    }
    if (m_split_check_phase == SplitCheckPhase::LEFT)
    {
        if (m_tp->lft == -2)
        {
            m_tp->lft = tess_check_col(m_tp->x1, m_tp->y1, m_tp->y2);
        }
        m_split_check_phase = SplitCheckPhase::RIGHT;
    }
    if (m_tp->lft != m_tp->top)
    {
        needed = true;
        reset_split_check();
        return true;
    }
    if (m_split_check_phase == SplitCheckPhase::RIGHT)
    {
        if (m_tp->rgt == -2)
        {
            m_tp->rgt = tess_check_col(m_tp->x2, m_tp->y1, m_tp->y2);
        }
        m_split_check_phase = SplitCheckPhase::MIDDLE;
    }
    if (m_tp->rgt != m_tp->top)
    {
        needed = true;
        reset_split_check();
        return true;
    }

    if (m_split_check_phase == SplitCheckPhase::MIDDLE)
    {
        if (m_tp->x2 - m_tp->x1 > m_tp->y2 - m_tp->y1)
        {
            // divide down the middle
            const int mid = (m_tp->x1 + m_tp->x2) >> 1; // Find mid-point
            start_column_probe(mid, m_tp->y1 + 1, m_tp->y2 - 1);
        }
        else
        {
            // divide across the middle
            const int mid = (m_tp->y1 + m_tp->y2) >> 1; // Find mid-point
            start_row_probe(m_tp->x1 + 1, m_tp->x2 - 1, mid);
        }
        return split_needed(needed);
    }
    reset_split_check();
    return true;
}

void Tesseral::start_fill_box()
{
    if (g_fill_color == 0)
    {
        finish_fill_box();
        return;
    }
    if (g_fill_color > 0)
    {
        m_tp->top = g_fill_color % g_colors;
    }

    const int columns = m_tp->x2 - m_tp->x1 - 1;
    if (m_guess_plot || columns < 2)
    {
        m_fill.mode = FillMode::PIXEL;
        m_fill.col = m_tp->x1 + 1;
        m_fill.row = m_tp->y1 + 1;
        return;
    }

    m_fill.mode = FillMode::SPAN;
    m_fill.row = m_tp->y1 + 1;
    m_fill.pixels.assign(columns, static_cast<Byte>(m_tp->top));
}

void Tesseral::finish_fill_box()
{
    m_fill.mode = FillMode::NONE;
    m_fill.col = 0;
    m_fill.row = 0;
    --m_tp;
}

bool Tesseral::fill_box()
{
    // all 4 edges are the same color, fill in
    if (m_fill.mode == FillMode::NONE)
    {
        start_fill_box();
    }
    if (m_fill.mode == FillMode::NONE)
    {
        return true;
    }

    if (m_fill.mode == FillMode::PIXEL)
    {
        if (m_fill.col >= m_tp->x2 || m_fill.row >= m_tp->y2)
        {
            finish_fill_box();
            return true;
        }
        for (int count = 0; count < TESSERAL_PIXEL_FILL_CHUNK && m_fill.col < m_tp->x2; ++count)
        {
            g_col = m_fill.col;
            g_row = m_fill.row;
            g_plot(g_col, g_row, m_tp->top);

            ++m_fill.row;
            if (m_fill.row >= m_tp->y2)
            {
                m_fill.row = m_tp->y1 + 1;
                ++m_fill.col;
            }
        }
        if (m_fill.col >= m_tp->x2)
        {
            finish_fill_box();
            return true;
        }
        return false;
    }

    for (int count = 0; count < TESSERAL_SPAN_FILL_CHUNK && m_fill.row < m_tp->y2; ++count)
    {
        // use write_span for speed
        g_row = m_fill.row;
        write_span(g_row, m_tp->x1 + 1, m_tp->x2 - 1, m_fill.pixels.data());
        if (g_plot != g_put_color) // symmetry
        {
            if (const int k = g_stop_pt.y - (g_row - g_start_pt.y); k > g_i_stop_pt.y && k < g_logical_screen.y_dots)
            {
                write_span(k, m_tp->x1 + 1, m_tp->x2 - 1, m_fill.pixels.data());
            }
        }
        ++m_fill.row;
    }
    if (m_fill.row >= m_tp->y2)
    {
        finish_fill_box();
        return true;
    }
    return false;
}

bool Tesseral::split_box()
{
    // box not surrounded by same color, subdivide
    if (!m_split_box_active)
    {
        m_split_box_vertical = m_tp->x2 - m_tp->x1 > m_tp->y2 - m_tp->y1;
        if (m_split_box_vertical)
        {
            // divide down the middle
            m_split_box_mid = (m_tp->x1 + m_tp->x2) >> 1; // Find mid-point
            start_column_probe(m_split_box_mid, m_tp->y1 + 1, m_tp->y2 - 1);
        }
        else
        {
            // divide across the middle
            m_split_box_mid = (m_tp->y1 + m_tp->y2) >> 1; // Find mid-point
            start_row_probe(m_tp->x1 + 1, m_tp->x2 - 1, m_split_box_mid);
        }
        m_split_box_active = true;
    }

    int mid_color;
    if (!finish_edge_probe(mid_color))
    {
        return false;
    }
    m_split_box_active = false;

    Tess *tp2;
    if (m_split_box_vertical)
    {
        if (m_tp->x2 - m_split_box_mid > 1)
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
            if (m_split_box_mid - m_tp->x1 > 1)
            {
                // left part >= 1 col, stack right
                *++m_tp = *tp2; // copy current box
                m_tp->x2 = m_split_box_mid;
                m_tp->rgt = mid_color;
            }
            tp2->x1 = m_split_box_mid;
            tp2->lft = mid_color;
        }
        else
        {
            --m_tp;
        }
    }
    else
    {
        if (m_tp->y2 - m_split_box_mid > 1)
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
            if (m_split_box_mid - m_tp->y1 > 1)
            {
                // top also >= 1 col, stack bottom
                *++m_tp = *tp2; // copy current box
                m_tp->y2 = m_split_box_mid;
                m_tp->bot = mid_color;
            }
            tp2->y1 = m_split_box_mid;
            tp2->top = mid_color;
        }
        else
        {
            --m_tp;
        }
    }
    return true;
}

void Tesseral::suspend()
{
    if (m_init_phase != InitPhase::COMPLETE)
    {
        add_work_list(
            g_start_pt.x, g_start_pt.y, g_stop_pt.x, g_stop_pt.y, g_start_pt.x, g_start_pt.y, 0, g_work_symmetry);
        return;
    }

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

} // namespace id::engine
