// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

struct Viewport
{
    bool enabled{};             // false for full screen, true for window
    bool keep_aspect_ratio{};   // true to keep virtual aspect
    bool crop{};                // true to crop default coords
    bool z_scroll{};            // screen/zoom box false fixed, true relaxed
    float final_aspect_ratio{}; // for view shape and rotation
    float reduction{};          // window auto-sizing
    int x_dots{};               // explicit view sizing
    int y_dots{};               //
};

extern Viewport g_viewport;

} // namespace id::engine
