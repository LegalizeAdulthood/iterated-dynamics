// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string_view>

namespace id::engine
{

struct VideoInfo;

// TODO: removed later
// Parameter processing keeps key-name parsing here until slice 53.5.
// returns key number for the passed key name, 0 if not a key name
int check_vid_mode_key_name(std::string_view key_name);

// true if a disk video mode can be represented by a GIF image
bool is_valid_disk_video_mode(const VideoInfo &mode);

// true if a display video mode fits the physical display
bool is_valid_display_video_mode(const VideoInfo &mode, int max_width, int max_height);

} // namespace id::engine
