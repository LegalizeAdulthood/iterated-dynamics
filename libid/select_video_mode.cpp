// SPDX-License-Identifier: GPL-3.0-only
//
#include "select_video_mode.h"

#include "port.h"
#include "prototyp.h"

#include "drivers.h"
#include "find_path.h"
#include "full_screen_choice.h"
#include "get_key_no_help.h"
#include "helpdefs.h"
#include "id_data.h"
#include "load_config.h"
#include "save_file.h"
#include "stop_msg.h"
#include "value_saver.h"
#include "video_mode.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <string>

namespace fs = std::filesystem;

static std::vector<int> s_entry_nums;
static bool s_modes_changed{};

static int check_modekey(int curkey, int choice);
static bool ent_less(int lhs, int rhs);
static void update_id_cfg();

inline bool is_writable(const std::string &path)
{
    const fs::perms read_write = fs::perms::owner_read | fs::perms::owner_write;
    return (fs::status(path).permissions() & read_write) == read_write;
}

static void format_vid_table(int choice, char *buf)
{
    char kname[5];
    const int idx = s_entry_nums[choice];
    assert(idx < g_video_table_len);
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[idx],
           sizeof(g_video_entry));
    vidmode_keyname(g_video_entry.keynum, kname);
    std::sprintf(buf, "%-5s %-12s %5d %5d %3d  %.12s %.26s", // 34 chars
        kname, g_video_entry.driver->get_description().c_str(), g_video_entry.xdots, g_video_entry.ydots,
        g_video_entry.colors, g_video_entry.driver->get_name().c_str(), g_video_entry.comment);
}

int select_video_mode(int curmode)
{
    std::vector<int> attributes;
    int ret;

    attributes.resize(g_video_table_len);
    s_entry_nums.resize(g_video_table_len);
    // init tables
    std::fill(attributes.begin(), attributes.end(), 1);
    std::iota(s_entry_nums.begin(), s_entry_nums.end(), 0);
    std::sort(s_entry_nums.begin(), s_entry_nums.end(), ent_less);

    // pick default mode
    if (curmode < 0)
    {
        g_video_entry.colors = 256;
    }
    else
    {
        std::memcpy((char *) &g_video_entry, (char *) &g_video_table[curmode], sizeof(g_video_entry));
    }
    int i;
    for (i = 0; i < g_video_table_len; ++i)  // find default mode
    {
        if (g_video_entry.colors == g_video_table[s_entry_nums[i]].colors
            && (curmode < 0
                || std::memcmp((char *) &g_video_entry, (char *) &g_video_table[s_entry_nums[i]], sizeof(g_video_entry)) == 0))
        {
            break;
        }
    }
    if (i >= g_video_table_len) // no match, default to first entry
    {
        i = 0;
    }

    {
        ValueSaver saved_tab_mode{g_tab_mode, false};
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_VIDEO_MODE};
        s_modes_changed = false;
        i = full_screen_choice(CHOICE_HELP, "Select Video Mode",
            "key...name..........xdot..ydot.colr.driver......comment......", nullptr, g_video_table_len,
            nullptr, attributes.data(), 1, 16, 74, i, format_vid_table, nullptr, nullptr,
            check_modekey);
    }
    if (i == -1)
    {
        // update id.cfg for new key assignments
        if (s_modes_changed
            && g_bad_config == config_status::OK
            && stopmsg(stopmsg_flags::CANCEL | stopmsg_flags::NO_BUZZER | stopmsg_flags::INFO_ONLY,
                "Save new function key assignments or cancel changes?") == 0)
        {
            update_id_cfg();
        }
        return -1;
    }
    // picked by function key or ENTER key
    i = (i < 0) ? (-1 - i) : s_entry_nums[i];
    // the selected entry now in g_video_entry
    std::memcpy((char *) &g_video_entry, (char *) &g_video_table[i], sizeof(g_video_entry));

    // copy id.cfg table to resident table, note selected entry
    int k = 0;
    for (i = 0; i < g_video_table_len; ++i)
    {
        if (g_video_table[i].keynum > 0)
        {
            if (std::memcmp((char *)&g_video_entry, (char *)&g_video_table[i], sizeof(g_video_entry)) == 0)
            {
                k = g_video_table[i].keynum;
            }
        }
    }
    ret = k;
    if (k == 0)  // selected entry not a copied (assigned to key) one
    {
        std::memcpy((char *)&g_video_table[MAX_VIDEO_MODES-1],
               (char *)&g_video_entry, sizeof(*g_video_table));
        ret = 1400; // special value for check_vidmode_key
    }

    // update id.cfg for new key assignments
    if (s_modes_changed && g_bad_config == config_status::OK)
    {
        update_id_cfg();
    }

    return ret;
}

static int check_modekey(int curkey, int choice)
{
    int i = check_vidmode_key(1, curkey);
    if (i >= 0)
    {
        return -1-i;
    }
    i = s_entry_nums[choice];
    int ret = 0;
    if ((curkey == '-' || curkey == '+')
        && (g_video_table[i].keynum == 0 || g_video_table[i].keynum >= 1084))
    {
        if (g_bad_config != config_status::OK)
        {
            stopmsg("Missing or bad id.cfg file. Can't reassign keys.");
        }
        else
        {
            if (curkey == '-')
            {
                // deassign key?
                if (g_video_table[i].keynum >= 1084)
                {
                    g_video_table[i].keynum = 0;
                    s_modes_changed = true;
                }
            }
            else
            {
                // assign key?
                int j = get_a_key_no_help();
                if (j >= 1084 && j <= 1113)
                {
                    for (int k = 0; k < g_video_table_len; ++k)
                    {
                        if (g_video_table[k].keynum == j)
                        {
                            g_video_table[k].keynum = 0;
                            ret = -1; // force redisplay
                        }
                    }
                    g_video_table[i].keynum = j;
                    s_modes_changed = true;
                }
            }
        }
    }
    return ret;
}

static bool ent_less(const int lhs, const int rhs)
{
    int i = g_video_table[lhs].keynum;
    if (i == 0)
    {
        i = 9999;
    }
    int j = g_video_table[rhs].keynum;
    if (j == 0)
    {
        j = 9999;
    }
    return i < j || i == j && lhs < rhs;
}

static void update_id_cfg()
{
    char buf[121];
    char kname[5];
    std::FILE *cfgfile;
    std::FILE *outfile;
    int i;
    int j;
    int linenum;
    int nextlinenum;
    int nextmode;
    VIDEOINFO vident{};

    const std::string cfgname = find_path("id.cfg");

    if (!is_writable(cfgname))
    {
        std::snprintf(buf, std::size(buf), "Can't write %s", cfgname.c_str());
        stopmsg(buf);
        return;
    }
    const std::string outname{(fs::path{cfgname}.parent_path() / "id.tmp").string()};
    outfile = open_save_file(outname, "w");
    if (outfile == nullptr)
    {
        std::snprintf(buf, std::size(buf), "Can't create %s", outname.c_str());
        stopmsg(buf);
        return;
    }
    cfgfile = std::fopen(cfgname.c_str(), "r");

    nextmode = 0;
    linenum = nextmode;
    nextlinenum = g_cfg_line_nums[0];
    while (std::fgets(buf, std::size(buf), cfgfile))
    {
        char colorsbuf[10];
        ++linenum;
        // replace this line?
        if (linenum == nextlinenum)
        {
            vident = g_video_table[nextmode];
            vidmode_keyname(vident.keynum, kname);
            std::snprintf(colorsbuf, std::size(colorsbuf), "%3d", vident.colors);
            std::fprintf(outfile, "%-4s,%4d,%5d,%s,%s,%s\n",
                    kname,
                    vident.xdots,
                    vident.ydots,
                    colorsbuf,
                    vident.driver->get_name().c_str(),
                    vident.comment);
            if (++nextmode >= g_video_table_len)
            {
                nextlinenum = 32767;
            }
            else
            {
                nextlinenum = g_cfg_line_nums[nextmode];
            }
        }
        else
        {
            std::fputs(buf, outfile);
        }
    }

    std::fclose(cfgfile);
    std::fclose(outfile);
    fs::remove(cfgname);         // success assumed on these lines
    fs::rename(outname, cfgname); // since we checked earlier with access
}

void request_video_mode(int &kbd_char)
{
    driver_stack_screen();
    kbd_char = select_video_mode(g_adapter);
    if (check_vidmode_key(0, kbd_char) >= 0) // picked a new mode?
    {
        driver_discard_screen();
    }
    else
    {
        driver_unstack_screen();
    }
}
