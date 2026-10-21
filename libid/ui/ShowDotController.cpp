// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ShowDotController.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/pixel.h"
#include "engine/show_dot.h"
#include "engine/StandardFractal.h"
#include "engine/VideoInfo.h"
#include "misc/Driver.h"
#include "ui/find_special_colors.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <thread>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{

ShowDotController::ShowDotController() :
    m_delay{std::max(g_orbit_delay, 0)},
    m_enabled{g_show_dot >= 0}
{
    if (!m_enabled)
    {
        return;
    }
    if (g_colors <= 0 || g_logical_screen.x_dots <= 0 || g_logical_screen.y_dots <= 0)
    {
        throw std::logic_error{"Show-dot requires image dimensions and a palette"};
    }
    find_special_colors();
    switch (g_auto_show_dot)
    {
    case AutoShowDot::DARK:
        m_color = g_color_dark % g_colors;
        break;
    case AutoShowDot::MEDIUM:
        m_color = g_color_medium % g_colors;
        break;
    case AutoShowDot::BRIGHT:
    case AutoShowDot::AUTOMATIC:
        m_color = g_color_bright % g_colors;
        break;
    case AutoShowDot::NONE:
        m_color = g_show_dot % g_colors;
        break;
    }
    if (g_size_dot > 0)
    {
        m_width = static_cast<int>(std::min(150.0, static_cast<double>(g_size_dot) * g_logical_screen.x_dots / 1024.0));
        while (m_width * m_width + 5 * m_width + 4 > 1000)
        {
            --m_width;
        }
    }
    m_saved.reserve(static_cast<std::size_t>((m_width + 2) * (m_width + 3) * 2));
}

ShowDotController::~ShowDotController()
{
    restore();
}

bool ShowDotController::enabled() const
{
    return m_enabled;
}

void ShowDotController::save_pixel(const int col, const int row)
{
    if (col < 0 || row < 0 || col >= g_logical_screen.x_dots || row >= g_logical_screen.y_dots)
    {
        return;
    }
    m_saved.push_back({{col, row}, get_color(col, row)});
    put_color_a(col, row, m_color);
}

void ShowDotController::plot(int col, const int row)
{
    if (g_plot == no_plot)
    {
        return;
    }
    const bool periodic{g_plot == sym_pi_plot || g_plot == sym_pi_plot2j || g_plot == sym_pi_plot4j};
    if (periodic && g_pi_in_pixels <= 0)
    {
        throw std::logic_error{"Periodic show-dot symmetry requires a positive pixel period"};
    }
    const bool both_axes{g_plot == sym_plot4 || g_plot == sym_plot4_basin || g_plot == sym_pi_plot4j};
    const bool y_axis{both_axes || g_plot == sym_plot2y};
    const bool x_axis{both_axes || g_plot == sym_plot2 || g_plot == sym_plot2_basin};
    const bool origin{g_plot == sym_plot2j || g_plot == sym_pi_plot2j};
    const int last_col{g_plot == sym_pi_plot4j ? (g_start_pt.x + g_stop_pt.x) / 2 : g_stop_pt.x};
    do
    {
        save_pixel(col, row);
        const int mirror_col{g_stop_pt.x - (col - g_start_pt.x)};
        const int mirror_row{g_stop_pt.y - (row - g_start_pt.y)};
        if (y_axis)
        {
            save_pixel(mirror_col, row);
        }
        if (mirror_row > g_i_stop_pt.y)
        {
            if (x_axis)
            {
                save_pixel(col, mirror_row);
            }
            if (both_axes || origin)
            {
                save_pixel(mirror_col, mirror_row);
            }
        }
        if (!periodic)
        {
            break;
        }
        col += g_pi_in_pixels;
    } while (col <= last_col);
}

void ShowDotController::show(const math::Point2i point)
{
    if (!m_saved.empty())
    {
        throw std::logic_error{"Show-dot marker must be restored before showing another"};
    }
    if (!m_enabled)
    {
        return;
    }
    const int width{m_width + 1};
    int left{point.x};
    int right{point.x};
    int first_row{point.y};
    int last_row{point.y};
    int row_step{1};
    bool shift_left_edge{true};
    if (width > 0)
    {
        if (point.x + width <= g_i_stop_pt.x && point.y + width <= g_i_stop_pt.y)
        {
            right += width;
            first_row += width;
            last_row += 1;
            row_step = -1;
        }
        else if (point.x - width >= g_i_start_pt.x && point.y + width <= g_i_stop_pt.y)
        {
            left -= width;
            first_row += width;
            last_row += 1;
            row_step = -1;
        }
        else if (point.x - width >= g_i_start_pt.x && point.y - width >= g_i_start_pt.y)
        {
            left -= width;
            first_row -= width;
            last_row -= 1;
        }
        else if (point.x + width <= g_i_stop_pt.x && point.y - width >= g_i_start_pt.y)
        {
            right += width;
            first_row -= width;
            last_row -= 1;
            shift_left_edge = false;
        }
    }
    if (first_row != point.y)
    {
        for (int row = first_row; row != last_row + row_step; row += row_step)
        {
            for (int col = left; col <= right; ++col)
            {
                plot(col, row);
            }
            if (shift_left_edge)
            {
                ++left;
            }
            else
            {
                --right;
            }
        }
    }
    plot(point.x, point.y);
}

void ShowDotController::restore()
{
    // Reverse order also restores overlapping symmetry copies exactly.
    for (std::vector<SavedPixel>::const_reverse_iterator pixel = m_saved.crbegin(); pixel != m_saved.crend(); ++pixel)
    {
        put_color_a(pixel->point.x, pixel->point.y, pixel->color);
    }
    m_saved.clear();
}

void ShowDotController::wait() const
{
    if (m_delay > 0)
    {
        driver_flush();
        driver_delay(0);
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>(m_delay) * 100));
        driver_delay(0);
    }
}

void ShowDotController::iterate(StandardFractal &calculation)
{
    calculation.begin_chunk();
    while (calculation.select_pixel())
    {
        const math::Point2i point{calculation.selected_pixel()};
        if (!m_active_pixel)
        {
            show(point);
            wait();
            restore();
            m_active_pixel = point;
        }
        else if (*m_active_pixel != point)
        {
            throw std::logic_error{"Show-dot calculation changed an unfinished pixel"};
        }
        if (!calculation.calculate_pixel())
        {
            static_cast<void>(calculation.consume_traversal_yield());
            break;
        }
        m_active_pixel.reset();
        const bool yielded{calculation.consume_traversal_yield()};
        if (m_delay > 0 || yielded)
        {
            break;
        }
    }
}

} // namespace id::ui
