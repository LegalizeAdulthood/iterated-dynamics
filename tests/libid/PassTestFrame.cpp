// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "PassTestFrame.h"

#include <engine/calcfrac.h>
#include <engine/VideoInfo.h>

using namespace id::engine;

namespace id::test
{

PassTestFrame::PassTestFrame() :
    m_saved_screen{g_logical_screen},
    m_saved_width{g_screen_x_dots},
    m_saved_height{g_screen_y_dots},
    m_saved_start{g_start_pt},
    m_saved_stop{g_stop_pt},
    m_saved_i_start{g_i_start_pt},
    m_saved_i_stop{g_i_stop_pt},
    m_saved_begin{g_begin_pt}
{
}

void PassTestFrame::reset(const int width, const int height)
{
    m_width = width;
    g_screen_x_dots = width;
    g_screen_y_dots = height;
    g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
    g_start_pt = {0, 0};
    g_stop_pt = {width - 1, height - 1};
    g_i_start_pt = g_start_pt;
    g_i_stop_pt = g_stop_pt;
    g_begin_pt = g_start_pt;
    m_pixels.assign(width * height, 0);
    m_plots.clear();
}

int PassTestFrame::read_pixel(const int col, const int row) const
{
    return m_pixels.at(row * m_width + col);
}

void PassTestFrame::write_pixel(const int col, const int row, const int color)
{
    m_pixels.at(row * m_width + col) = color;
    m_plots.push_back({col, row});
}

int PassTestFrame::width() const
{
    return m_width;
}

std::vector<int> &PassTestFrame::pixels()
{
    return m_pixels;
}

const std::vector<math::Point2i> &PassTestFrame::plots() const
{
    return m_plots;
}

} // namespace id::test
