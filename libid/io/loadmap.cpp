// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/loadmap.h"

#include "engine/color_state.h"
#include "io/find_path.h"
#include "io/has_ext.h"
#include "io/merge_path_names.h"
#include "misc/version.h"
#include "engine/cmdfiles.h"
#include "ui/rotate.h"
#include "ui/stop_msg.h"

#include <config/path_limits.h>
#include <config/port.h>

#include <array> // std::size
#include <cstdio>
#include <cstring>

//*************************************************************************

namespace
{

#ifdef _WIN32
#pragma pack(push, 1)
#define ID_PACKED
#else
#define ID_PACKED __attribute__((packed))
#endif
struct PaletteType
{
    Byte red;
    Byte green;
    Byte blue;
} ID_PACKED;
#ifdef _WIN32
#pragma pack(pop)
#endif

} // namespace

#define DAC ((PaletteType *)g_dac_box)

bool validate_luts(const char *map_name)
{
    unsigned r;
    unsigned g;
    unsigned b;
    char    temp[ID_FILE_MAX_PATH+1];
    char    temp_fn[ID_FILE_MAX_PATH];
    std::strcpy(temp, g_map_name.c_str());
    std::strcpy(temp_fn, map_name);
    merge_path_names(temp, temp_fn, CmdFile::AT_CMD_LINE);
    if (!has_ext(temp))   // Did name have an extension?
    {
        std::strcat(temp, ".map");  // No? Then add .map
    }
    std::FILE *f = std::fopen(find_path(temp).c_str(), "r"); // search the dos path
    if (f == nullptr)
    {
        char line[160];
        std::snprintf(line, std::size(line), "Could not load color map %s", map_name);
        stop_msg(line);
        return true;
    }
    unsigned index;
    for (index = 0; index < 256; index++)
    {
        char line[160];
        if (std::fgets(line, std::size(line), f) == nullptr)
        {
            break;
        }
        std::sscanf(line, "%u %u %u", &r, &g, &b);
        //* load global dac values *
        DAC[index].red = static_cast<Byte>(r % 256);
        DAC[index].green = static_cast<Byte>(g % 256);
        DAC[index].blue = static_cast<Byte>(b % 256);
    }
    std::fclose(f);
    while (index < 256)
    {
        // zap unset entries
        DAC[index].green = 40;
        DAC[index].blue = 40;
        DAC[index].red = 40;
        ++index;
    }
    g_color_state = ColorState::MAP_FILE;
    g_color_file = map_name;
    return false;
}

//*************************************************************************

void set_color_palette_name(const char *fn)
{
    if (validate_luts(fn))
    {
        return;
    }
    for (int i = 0; i < 256; ++i)
    {
        g_map_clut[i][0] = g_dac_box[i][0];
        g_map_clut[i][1] = g_dac_box[i][1];
        g_map_clut[i][2] = g_dac_box[i][2];
    }
    g_map_specified = true;
}
