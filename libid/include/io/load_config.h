// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "ui/video_mode.h"

#include <string>

namespace id::io
{

enum class ConfigStatus
{
    OK = 0,
    BAD_WITH_MESSAGE = 1,
    BAD_NO_MESSAGE = -1
};

extern ConfigStatus          g_bad_config;
extern int                   g_cfg_line_nums[ui::MAX_VIDEO_MODES];

void load_config();
void load_config(const std::string &cfg_path);

} // namespace id::io
