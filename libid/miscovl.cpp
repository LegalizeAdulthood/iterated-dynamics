/*
        Overlayed odds and ends that don't fit anywhere else.
*/
#include "port.h"
#include "prototyp.h"

#include "miscovl.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "drivers.h"
#include "find_path.h"
#include "fractalp.h"
#include "framain2.h"
#include "full_screen_choice.h"
#include "get_key_no_help.h"
#include "helpdefs.h"
#include "id_data.h"
#include "load_config.h"
#include "rotate.h"
#include "stop_msg.h"
#include "video_mode.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static int check_modekey(int curkey, int choice);
static bool ent_less(int lhs, int rhs);
static void update_id_cfg();


// JIIM

inline bool is_writable(const std::string &path)
{
    const fs::perms read_write = fs::perms::owner_read | fs::perms::owner_write;
    return (fs::status(path).permissions() & read_write) == read_write;
}

static std::array<int, MAX_VIDEO_MODES> entnums;
static bool modes_changed = false;

static void format_vid_table(int choice, char *buf)
{
    char local_buf[81];
    char kname[5];
    int truecolorbits;
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[entnums[choice]],
           sizeof(g_video_entry));
    vidmode_keyname(g_video_entry.keynum, kname);
    std::sprintf(buf, "%-5s %-25s %5d %5d ",  // 44 chars
            kname, g_video_entry.name, g_video_entry.xdots, g_video_entry.ydots);
    truecolorbits = g_video_entry.dotmode/1000;
    if (truecolorbits == 0)
    {
        std::snprintf(local_buf, NUM_OF(local_buf), "%s%3d",  // 47 chars
                buf, g_video_entry.colors);
    }
    else
    {
        std::snprintf(local_buf, NUM_OF(local_buf), "%s%3s",  // 47 chars
                buf, (truecolorbits == 4)?" 4g":
                (truecolorbits == 3)?"16m":
                (truecolorbits == 2)?"64k":
                (truecolorbits == 1)?"32k":"???");
    }
    std::sprintf(buf, "%s %.12s %.12s",  // 74 chars
            local_buf, g_video_entry.driver->name, g_video_entry.comment);
}

int select_video_mode(int curmode)
{
    int attributes[MAX_VIDEO_MODES];
    int ret;

    for (int i = 0; i < g_video_table_len; ++i)  // init tables
    {
        entnums[i] = i;
        attributes[i] = 1;
    }
    std::sort(entnums.begin(), entnums.end(), ent_less);

    // pick default mode
    if (curmode < 0)
    {
        g_video_entry.videomodeax = 19;  // vga
        g_video_entry.colors = 256;
    }
    else
    {
        std::memcpy((char *) &g_video_entry, (char *) &g_video_table[curmode], sizeof(g_video_entry));
    }
    int i;
    for (i = 0; i < g_video_table_len; ++i)  // find default mode
    {
        if (g_video_entry.videomodeax == g_video_table[entnums[i]].videomodeax
            && g_video_entry.colors == g_video_table[entnums[i]].colors
            && (curmode < 0
                || std::memcmp((char *) &g_video_entry, (char *) &g_video_table[entnums[i]], sizeof(g_video_entry)) == 0))
        {
            break;
        }
    }
    if (i >= g_video_table_len) // no match, default to first entry
    {
        i = 0;
    }

    bool const old_tab_mode = g_tab_mode;
    help_labels const old_help_mode = g_help_mode;
    modes_changed = false;
    g_tab_mode = false;
    g_help_mode = help_labels::HELPVIDSEL;
    i = fullscreen_choice(CHOICE_HELP,
                          "Select Video Mode",
                          "key...name.......................xdot..ydot.colr.driver......comment......",
                          nullptr, g_video_table_len, nullptr, attributes,
                          1, 16, 74, i, format_vid_table, nullptr, nullptr, check_modekey);
    g_tab_mode = old_tab_mode;
    g_help_mode = old_help_mode;
    if (i == -1)
    {
        // update id.cfg for new key assignments
        if (modes_changed
            && g_bad_config == config_status::OK
            && stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER | STOPMSG_INFO_ONLY,
                "Save new function key assignments or cancel changes?") == 0)
        {
            update_id_cfg();
        }
        return -1;
    }
    // picked by function key or ENTER key
    i = (i < 0) ? (-1 - i) : entnums[i];
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
    if (modes_changed && g_bad_config == config_status::OK)
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
    i = entnums[choice];
    int ret = 0;
    if ((curkey == '-' || curkey == '+')
        && (g_video_table[i].keynum == 0 || g_video_table[i].keynum >= 1084))
    {
        if (g_bad_config != config_status::OK)
        {
            stopmsg(STOPMSG_NONE, "Missing or bad id.cfg file. Can't reassign keys.");
        }
        else
        {
            if (curkey == '-')
            {
                // deassign key?
                if (g_video_table[i].keynum >= 1084)
                {
                    g_video_table[i].keynum = 0;
                    modes_changed = true;
                }
            }
            else
            {
                // assign key?
                int j = getakeynohelp();
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
                    modes_changed = true;
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
#ifndef XFRACT
    char buf[121], kname[5];
    std::FILE *cfgfile, *outfile;
    int i, j, linenum, nextlinenum, nextmode;
    VIDEOINFO vident;

    const std::string cfgname = find_path("id.cfg");

    if (!is_writable(cfgname))
    {
        std::snprintf(buf, NUM_OF(buf), "Can't write %s", cfgname.c_str());
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    const std::string outname{(fs::path{cfgname}.parent_path() / "id.tmp").string()};
    outfile = std::fopen(outname.c_str(), "w");
    if (outfile == nullptr)
    {
        std::snprintf(buf, NUM_OF(buf), "Can't create %s", outname.c_str());
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    cfgfile = std::fopen(cfgname.c_str(), "r");

    nextmode = 0;
    linenum = nextmode;
    nextlinenum = g_cfg_line_nums[0];
    while (std::fgets(buf, NUM_OF(buf), cfgfile))
    {
        char colorsbuf[10];
        ++linenum;
        if (linenum == nextlinenum)
        {
            // replace this line
            std::memcpy((char *)&vident, (char *)&g_video_table[nextmode],
                   sizeof(g_video_entry));
            vidmode_keyname(vident.keynum, kname);
            std::strcpy(buf, vident.name);
            i = (int) std::strlen(buf);
            while (i && buf[i-1] == ' ') // strip trailing spaces to compress
            {
                --i;
            }
            j = i + 5;
            while (j < 32)
            {
                // tab to column 33
                buf[i++] = '\t';
                j += 8;
            }
            buf[i] = 0;
            int truecolorbits = vident.dotmode/1000;
            if (truecolorbits == 0)
            {
                std::snprintf(colorsbuf, NUM_OF(colorsbuf), "%3d", vident.colors);
            }
            else
            {
                std::snprintf(colorsbuf, NUM_OF(colorsbuf), "%3s",
                        (truecolorbits == 4)?" 4g":
                        (truecolorbits == 3)?"16m":
                        (truecolorbits == 2)?"64k":
                        (truecolorbits == 1)?"32k":"???");
            }
            std::fprintf(outfile, "%-4s,%s,%4x,%4x,%4x,%4x,%4d,%5d,%5d,%s,%s\n",
                    kname,
                    buf,
                    vident.videomodeax,
                    vident.videomodebx,
                    vident.videomodecx,
                    vident.videomodedx,
                    vident.dotmode%1000, // remove true-color flag, keep g_text_safe
                    vident.xdots,
                    vident.ydots,
                    colorsbuf,
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
#endif
}
