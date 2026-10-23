// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/video_mode.h"

#include "engine/pixel_limits.h"
#include "engine/VideoInfo.h"

namespace id::engine
{

bool is_valid_disk_video_mode(const VideoInfo &mode)
{
    return mode.colors == 256              //
        && mode.x_dots > 0                 //
        && mode.y_dots > 0                 //
        && mode.x_dots <= GIF_MAX_PIXELS   //
        && mode.y_dots <= GIF_MAX_PIXELS;
}

bool is_valid_display_video_mode(const VideoInfo &mode, const int max_width, const int max_height)
{
    return mode.colors == 256         //
        && mode.x_dots > 0            //
        && mode.y_dots > 0            //
        && max_width > 0              //
        && max_height > 0             //
        && mode.x_dots <= max_width   //
        && mode.y_dots <= max_height;
}

} // namespace id::engine
