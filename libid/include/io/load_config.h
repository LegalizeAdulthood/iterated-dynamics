// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/video_mode.h"

#include <string>

extern int                   g_cfg_line_nums[MAX_VIDEO_MODES];

void load_config();
void load_config(const std::string &cfg_path);
