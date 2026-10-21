// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include "ui/SaveImageProgress.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/VideoInfo.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <stdexcept>
#include <utility>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

namespace
{

int s_num_saves{};

} // namespace

SaveImageProgress::SaveImageProgress(const std::filesystem::path &filename) :
    m_disk{driver_is_disk()},
    m_width{g_logical_screen.x_dots},
    m_initial_color1{g_colors > 2 ? 2 : 0},
    m_initial_color2{g_colors > 2 ? 3 : 1}
{
    if ((++s_num_saves & 1) == 0)
    {
        std::swap(m_initial_color1, m_initial_color2);
    }
    m_color1 = m_initial_color1;
    m_color2 = m_initial_color2;
    if (m_disk)
    {
        dvid_status(1, "Saving " + filename.filename().string());
    }
}

SaveImageProgress::~SaveImageProgress()
{
    if (m_disk)
    {
        dvid_status(1, "");
    }
    else
    {
        int color1{m_initial_color1};
        int color2{m_initial_color2};
        for (int row = 0; row <= m_last_row; ++row)
        {
            next_colors(row, color1, color2);
            toggle_row(row, color1, color2);
        }
    }
    driver_flush();
}

void SaveImageProgress::next_colors(const int row, int &color1, int &color2)
{
    if ((row & 4) == 0)
    {
        if (++color1 >= g_colors)
        {
            color1 = 0;
        }
        if (++color2 >= g_colors)
        {
            color2 = 0;
        }
    }
}

void SaveImageProgress::toggle_row(const int row, const int color1, const int color2) const
{
    for (int edge = 0; 250 * edge < m_width; ++edge)
    {
        g_put_color(edge, row, get_color(edge, row) ^ color1);
        g_put_color(m_width - 1 - edge, row, get_color(m_width - 1 - edge, row) ^ color2);
    }
}

void SaveImageProgress::advance(const int row)
{
    if (row != m_last_row + 1)
    {
        throw std::logic_error("GIF save progress rows must be sequential");
    }
    if (m_disk)
    {
        display_disk_storage_progress();
    }
    else
    {
        next_colors(row, m_color1, m_color2);
        toggle_row(row, m_color1, m_color2);
    }
    m_last_row = row;
    driver_flush();
}

} // namespace id::ui
