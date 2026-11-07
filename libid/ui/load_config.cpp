// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/load_config.h>

#include <io/library.h>
#include <io/load_config.h>
#include <misc/Driver.h>
#include <ui/video_mode.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

ConfigStatus g_bad_config{};
int g_cfg_line_nums[MAX_VIDEO_MODES]{};

void load_config()
{
    load_config(find_file(ReadFile::ID_CONFIG, "id.cfg").string());
}

void load_config(const std::string &cfg_path)
{
    std::ifstream cfg_file{cfg_path};
    if (cfg_path.empty() || !cfg_file)
    {
        g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
        return;
    }

    std::fill_n(std::begin(g_cfg_line_nums), g_video_table_len, -1);
    VideoConfigReader reader{cfg_file};
    VideoConfigEntry entry;
    while (g_video_table_len < MAX_VIDEO_MODES && reader.next(entry))
    {
        const int key{check_vid_mode_key_name(entry.key_name)};
        if (key < 0)
        {
            g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
            return;
        }
        g_cfg_line_nums[g_video_table_len] = entry.line_num;

        VideoInfo video_entry{entry.x_dots, entry.y_dots, entry.colors, nullptr, {}};
        const std::size_t comment_len{std::min(entry.comment.size(), std::size(video_entry.comment) - 1)};
        std::copy_n(entry.comment.data(), comment_len, video_entry.comment);
        video_entry.driver = driver_find_by_name(entry.driver_name.c_str());
        if (video_entry.driver != nullptr && video_entry.driver->validate_mode(video_entry))
        {
            VideoInfo *begin{g_video_table};
            VideoInfo *end{g_video_table + g_video_table_len};
            const auto it = std::find_if(begin, end,
                [&](const VideoInfo &mode)
                {
                    return mode.driver == video_entry.driver //
                        && mode.colors == video_entry.colors //
                        && mode.x_dots == video_entry.x_dots //
                        && mode.y_dots == video_entry.y_dots;
                });
            if (it != end && g_video_mode_keys[it - begin] == 0)
            {
                g_video_mode_keys[it - begin] = key;
            }
            else
            {
                g_video_mode_keys[g_video_table_len] = key;
                add_video_mode(video_entry.driver, &video_entry);
            }
        }
    }
    if (reader.failed())
    {
        g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
    }
}

} // namespace id::ui
