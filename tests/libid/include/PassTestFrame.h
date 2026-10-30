// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <engine/LogicalScreen.h>
#include <math/Point.h>
#include <misc/ValueSaver.h>

#include <vector>

namespace id::test
{

class PassTestFrame
{
public:
    PassTestFrame();
    PassTestFrame(int width, int height);

    void reset(int width, int height);
    int read_pixel(int col, int row) const;
    void write_pixel(int col, int row, int color);
    int width() const;
    std::vector<int> &pixels();
    const std::vector<math::Point2i> &plots() const;

private:
    misc::ValueSaver<engine::LogicalScreen> m_saved_screen;
    misc::ValueSaver<int> m_saved_width;
    misc::ValueSaver<int> m_saved_height;
    misc::ValueSaver<math::Point2i> m_saved_start;
    misc::ValueSaver<math::Point2i> m_saved_stop;
    misc::ValueSaver<math::Point2i> m_saved_i_start;
    misc::ValueSaver<math::Point2i> m_saved_i_stop;
    misc::ValueSaver<math::Point2i> m_saved_begin;
    std::vector<int> m_pixels;
    std::vector<math::Point2i> m_plots;
    int m_width{};
};

} // namespace id::test
