// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

enum
{
    GIF_MAX_PIXELS = 65535, // Maximum pixel count across/down in a GIF image
    MAX_PIXELS = 32767,     // Signed 16-bit limit used by current image buffers
    OLD_MAX_PIXELS = 2048,  // Limit of some old fixed arrays
    MIN_PIXELS = 10         // Minimum pixel count across/down the screen
};

} // namespace id::engine
