// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/load_config.h"

#include "engine/pixel_limits.h"
#include "engine/video_mode.h"
#include "io/library.h"
#include "misc/Driver.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <iterator>
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
    std::FILE   *cfg_file;
    VideoInfo    video_entry;
    char        *fields[5]{};

    if (cfg_path.empty()                                             // can't find the file
        || (cfg_file = std::fopen(cfg_path.c_str(), "r")) == nullptr) // can't open it
    {
        g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
        return;
    }

    int line_num = 0;
    char temp_string[150];
    std::fill_n(std::begin(g_cfg_line_nums), g_video_table_len, -1);
    while (g_video_table_len < MAX_VIDEO_MODES
        && std::fgets(temp_string, std::size(temp_string), cfg_file))
    {
        if (std::strchr(temp_string, '\n') == nullptr)
        {
            // finish reading the line
            while (std::fgetc(cfg_file) != '\n' && !std::feof(cfg_file))
            {
            }
        }
        ++line_num;
        if (temp_string[0] == ';')
        {
            continue;   // comment line
        }
        temp_string[120] = 0;
        temp_string[std::strlen(temp_string) -1] = 0; // zap trailing \n
        int j = -1;
        int i = j;
        // key, 0: mode name, 1: x, 2: y, 3: colors, 4: driver, 5: comments
        while (true)
        {
            if (temp_string[++i] < ' ')
            {
                if (temp_string[i] == 0)
                {
                    break;
                }
                temp_string[i] = ' '; // convert tab (or whatever) to blank
            }
            else if (temp_string[i] == ',' && ++j < 6)
            {
                assert(j >= 0 && j < 11);
                fields[j] = &temp_string[i + 1]; // remember start of next field
                temp_string[i] = 0;              // make field a separate string
            }
        }
        const int key = check_vid_mode_key_name(temp_string);
        assert(fields[0]);
        const long x_dots = std::atol(fields[0]);
        assert(fields[1]);
        const long y_dots = std::atol(fields[1]);
        assert(fields[2]);
        const int colors = std::atoi(fields[2]);

        if (j < 4 ||
                key < 0 ||
                x_dots < MIN_PIXELS || x_dots > MAX_PIXELS ||
                y_dots < MIN_PIXELS || y_dots > MAX_PIXELS ||
                (colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
                 colors != 256)
           )
        {
            g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
            return;
        }
        g_cfg_line_nums[g_video_table_len] = line_num; // for update_id_cfg

        std::memset(&video_entry, 0, sizeof(video_entry));
        std::strncpy(&video_entry.comment[0], fields[4], std::size(video_entry.comment));
        video_entry.comment[25] = 0;
        video_entry.key      = key;
        video_entry.x_dots       = static_cast<short>(x_dots);
        video_entry.y_dots       = static_cast<short>(y_dots);
        video_entry.colors      = colors;

        // if valid, add to supported modes
        video_entry.driver = driver_find_by_name(fields[3]);
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
    std::fclose(cfg_file);
}

} // namespace id::io
