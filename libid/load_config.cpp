// SPDX-License-Identifier: GPL-3.0-only
//
#include "load_config.h"

#include "drivers.h"
#include "find_path.h"
#include "id_data.h"
#include "pixel_limits.h"
#include "video_mode.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int g_cfg_line_nums[MAX_VIDEO_MODES]{};

/* load_config
 *
 * Reads id.cfg, loading videoinfo entries into g_video_table.
 * Sets the number of entries, sets g_video_table_len.
 * Past g_video_table, g_cfg_line_nums are stored for update_id_cfg.
 * If id.cfg is not found or invalid, issues a message
 * (first time the problem occurs only, and only if options is
 * zero) and uses the hard-coded table.
 */
void load_config()
{
    load_config(find_path("id.cfg"));
}

void load_config(const std::string &cfg_path)
{
    std::FILE   *cfgfile;
    VideoInfo    vident;
    char        *fields[5]{};

    if (cfg_path.empty()                                             // can't find the file
        || (cfgfile = std::fopen(cfg_path.c_str(), "r")) == nullptr) // can't open it
    {
        g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
        return;
    }

    int linenum = 0;
    char tempstring[150];
    while (g_video_table_len < MAX_VIDEO_MODES
        && std::fgets(tempstring, std::size(tempstring), cfgfile))
    {
        if (std::strchr(tempstring, '\n') == nullptr)
        {
            // finish reading the line
            while (fgetc(cfgfile) != '\n' && !std::feof(cfgfile));
        }
        ++linenum;
        if (tempstring[0] == ';')
        {
            continue;   // comment line
        }
        tempstring[120] = 0;
        tempstring[(int) std::strlen(tempstring)-1] = 0; // zap trailing \n
        int j = -1;
        int i = j;
        // key, 0: mode name, 1: x, 2: y, 3: colors, 4: driver, 5: comments
        while (true)
        {
            if (tempstring[++i] < ' ')
            {
                if (tempstring[i] == 0)
                {
                    break;
                }
                tempstring[i] = ' '; // convert tab (or whatever) to blank
            }
            else if (tempstring[i] == ',' && ++j < 6)
            {
                assert(j >= 0 && j < 11);
                fields[j] = &tempstring[i+1]; // remember start of next field
                tempstring[i] = 0;   // make field a separate string
            }
        }
        int keynum = check_vid_mode_key_name(tempstring);
        assert(fields[0]);
        long xdots = std::atol(fields[0]);
        assert(fields[1]);
        long ydots = std::atol(fields[1]);
        assert(fields[2]);
        int colors = std::atoi(fields[2]);

        if (j < 4 ||
                keynum < 0 ||
                xdots < MIN_PIXELS || xdots > MAX_PIXELS ||
                ydots < MIN_PIXELS || ydots > MAX_PIXELS ||
                (colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
                 colors != 256)
           )
        {
            g_bad_config = ConfigStatus::BAD_NO_MESSAGE;
            return;
        }
        g_cfg_line_nums[g_video_table_len] = linenum; // for update_id_cfg

        std::memset(&vident, 0, sizeof(vident));
        std::strncpy(&vident.comment[0], fields[4], std::size(vident.comment));
        vident.comment[25] = 0;
        vident.key      = keynum;
        vident.x_dots       = (short)xdots;
        vident.y_dots       = (short)ydots;
        vident.colors      = colors;

        // if valid, add to supported modes
        vident.driver = driver_find_by_name(fields[3]);
        if (vident.driver != nullptr && vident.driver->validate_mode(&vident))
        {
            // look for a synonym mode and if found, overwrite its key
            VideoInfo *begin{&g_video_table[0]};
            VideoInfo *end{&g_video_table[g_video_table_len]};
            auto it = std::find_if(begin, end,
                [&](const VideoInfo &mode)
                {
                    return mode.driver == vident.driver //
                        && mode.colors == vident.colors //
                        && mode.x_dots == vident.x_dots   //
                        && mode.y_dots == vident.y_dots;
                });
            if (it != end && it->key == 0)
            {
                it->key = vident.key;
            }
            else
            {
                add_video_mode(vident.driver, &vident);
            }
        }
    }
    std::fclose(cfgfile);
}
