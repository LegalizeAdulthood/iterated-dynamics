// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/VideoInfo.h"

#include <array>
#include <string>

namespace id::ui
{

// Function-key bindings indexed by g_video_table entry; zero is unassigned.
extern std::array<int, engine::MAX_VIDEO_MODES> g_video_mode_keys;
// Binding copied with g_video_entry, including pending image restores.
extern int g_video_entry_key;

// Return the catalog entry for a bound key, or -1 when none is assigned.
int check_vid_mode_key(int key);

std::string vid_mode_key_name(int key);

} // namespace id::ui
