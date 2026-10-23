// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/video_mode.h"

#include "ui/id_keys.h"

using namespace id::engine;

namespace id::ui
{

int g_init_mode{};
std::array<int, MAX_VIDEO_MODES> g_video_mode_keys{};
int g_video_entry_key{};

int check_vid_mode_key(const int key)
{
    if (key == 1400)                // special value from select_vid_mode
    {
        return MAX_VIDEO_MODES - 1; // for last entry with no key assigned
    }
    if (key != 0)
    {
        // Search bindings for supported catalog entries.
        for (int i = 0; i < g_video_table_len; ++i)
        {
            if (g_video_mode_keys[i] == key)
            {
                return i;
            }
        }
    }
    return -1;
}

std::string vid_mode_key_name(int key)
{
    if (key > 0)
    {
        std::string name;
        if (key >= ID_KEY_ALT_F1 && key <= ID_KEY_ALT_F10)
        {
            name = 'A';
            key -= ID_KEY_ALT_F1 - 1;
        }
        else if (key >= ID_KEY_CTL_F1 && key <= ID_KEY_CTL_F10)
        {
            name = 'C';
            key -= ID_KEY_CTL_F1 - 1;
        }
        else if (key >= ID_KEY_SHF_F1 && key <= ID_KEY_SHF_F10)
        {
            name = 'S';
            key -= ID_KEY_SHF_F1 - 1;
        }
        else if (key >= ID_KEY_F1 && key <= ID_KEY_F10)
        {
            key -= ID_KEY_F1 - 1;
        }
        else
        {
            return {};
        }
        name += 'F';
        name += std::to_string(key);
        return name;
    }
    return {};
}

} // namespace id::ui
