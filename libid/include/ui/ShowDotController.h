// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/Point.h"

#include <optional>
#include <vector>

namespace id::engine
{
class StandardFractal;
}

namespace id::ui
{

class ShowDotController
{
public:
    ShowDotController();
    ~ShowDotController();
    ShowDotController(const ShowDotController &) = delete;
    ShowDotController(ShowDotController &&) = delete;
    ShowDotController &operator=(const ShowDotController &) = delete;
    ShowDotController &operator=(ShowDotController &&) = delete;

    bool enabled() const;
    void show(math::Point2i point);
    void restore();
    void iterate(engine::StandardFractal &calculation);

private:
    struct SavedPixel
    {
        math::Point2i point;
        int color;
    };

    void save_pixel(int col, int row);
    void plot(int col, int row);
    void wait() const;

    std::vector<SavedPixel> m_saved;
    std::optional<math::Point2i> m_active_pixel;
    int m_width{-1};
    int m_color{};
    int m_delay{};
    bool m_enabled{};
};

} // namespace id::ui
