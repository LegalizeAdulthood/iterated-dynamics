// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/load_config.h"

#include "engine/pixel_limits.h"
#include "engine/video_mode.h"
#include "io/library.h"
#include "misc/Driver.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::misc;

namespace id::io
{

ConfigStatus g_bad_config{}; // 'id.cfg' ok?
int g_cfg_line_nums[MAX_VIDEO_MODES]{};

/* load_config
 *
 * Reads id.cfg, loading VideoInfo entries into g_video_table.
 * Sets the number of entries, sets g_video_table_len.
 * Past g_video_table, g_cfg_line_nums are stored for update_id_cfg.
 * If id.cfg is not found or invalid, issues a message
 * (first time the problem occurs only, and only if options is
 * zero) and uses the hard-coded table.
 */
void load_config()
{
    load_config(find_file(ReadFile::ID_CONFIG, "id.cfg").string());
}

void load_config(const std::string &cfg_path)
{
    std::ifstream cfg_file{cfg_path};
    VideoInfo video_entry;

    if (cfg_path.empty() || !cfg_file)
    {
        g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
        return;
    }

    int line_num = 0;
    std::string line;
    std::fill_n(std::begin(g_cfg_line_nums), g_video_table_len, -1);
    while (g_video_table_len < MAX_VIDEO_MODES && std::getline(cfg_file, line))
    {
        ++line_num;
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (!line.empty() && line[0] == ';')
        {
            continue; // comment line
        }
        if (line.size() > 120)
        {
            line.resize(120);
        }
        for (char &ch : line)
        {
            if (static_cast<unsigned char>(ch) < ' ')
            {
                ch = ' '; // convert tab (or whatever) to blank
            }
        }
        // key, 0: x, 1: y, 2: colors, 3: driver, 4: comments
        std::string_view line_view{line};
        const std::size_t key_end{line_view.find(',')};
        if (key_end == std::string_view::npos)
        {
            g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
            return;
        }
        const std::string_view key_name{line_view.substr(0, key_end)};
        std::array<std::string_view, 5> fields{};
        std::size_t field_start{key_end + 1};
        for (int field = 0; field < 4; ++field)
        {
            const std::size_t field_end{line_view.find(',', field_start)};
            if (field_end == std::string_view::npos)
            {
                g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
                return;
            }
            fields[field] = line_view.substr(field_start, field_end - field_start);
            field_start = field_end + 1;
        }
        fields[4] = line_view.substr(field_start);

        const int key = check_vid_mode_key_name(key_name);
        const std::string x_field{fields[0]};
        const long x_dots = std::atol(x_field.c_str());
        const std::string y_field{fields[1]};
        const long y_dots = std::atol(y_field.c_str());
        const std::string colors_field{fields[2]};
        const int colors = std::atoi(colors_field.c_str());

        if (key < 0 || x_dots < MIN_PIXELS || x_dots > GIF_MAX_PIXELS || y_dots < MIN_PIXELS ||
            y_dots > GIF_MAX_PIXELS || (colors != 0 && colors != 2 && colors != 4 && colors != 16 && colors != 256))
        {
            g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
            return;
        }
        g_cfg_line_nums[g_video_table_len] = line_num; // for update_id_cfg

        video_entry = {};
        const std::size_t comment_len{std::min(fields[4].size(), std::size(video_entry.comment) - 1)};
        std::copy_n(fields[4].data(), comment_len, video_entry.comment);
        video_entry.key = key;
        video_entry.x_dots = static_cast<int>(x_dots);
        video_entry.y_dots = static_cast<int>(y_dots);
        video_entry.colors = colors;

        // if valid, add to supported modes
        const std::string driver_name{fields[3]};
        video_entry.driver = driver_find_by_name(driver_name.c_str());
        if (video_entry.driver != nullptr && video_entry.driver->validate_mode(video_entry))
        {
            // look for a synonym mode and if found, overwrite its key
            VideoInfo *begin{&g_video_table[0]};
            VideoInfo *end{&g_video_table[g_video_table_len]};
            const auto it = std::find_if(begin, end,
                [&](const VideoInfo &mode)
                {
                    return mode.driver == video_entry.driver //
                        && mode.colors == video_entry.colors //
                        && mode.x_dots == video_entry.x_dots //
                        && mode.y_dots == video_entry.y_dots;
                });
            if (it != end && it->key == 0)
            {
                it->key = video_entry.key;
            }
            else
            {
                add_video_mode(video_entry.driver, &video_entry);
            }
        }
    }
}

} // namespace id::io
