// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/VideoInfo.h"

namespace id::engine
{

int g_adapter{};
int g_init_mode{};
int g_screen_x_dots{};
int g_screen_y_dots{};
int g_colors{256};
VideoInfo g_video_entry{};
VideoInfo g_video_table[MAX_VIDEO_MODES]{};
int g_video_table_len{};

} // namespace id::engine
