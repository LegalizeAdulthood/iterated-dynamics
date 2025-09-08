// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/video_mode.h"

#include <string>

namespace id::io
{

extern int                   g_cfg_line_nums[ui::MAX_VIDEO_MODES];

void load_config();
void load_config(const std::string &cfg_path);

} // namespace id::io
