// SPDX-License-Identifier: GPL-3.0-only
//
#include "loadmap.h"

#include "cmdfiles.h"
#include "find_path.h"
#include "has_ext.h"
#include "id.h"
#include "merge_path_names.h"
#include "port.h"
#include "rotate.h"
#include "stop_msg.h"

#include <array> // std::size
#include <cstdio>
#include <cstring>

//*************************************************************************

namespace
{

struct PaletteType
{
    Byte red;
    Byte green;
    Byte blue;
};

} // namespace

#define dac ((PaletteType *)g_dac_box)

bool validate_luts(char const *fn)
{
    unsigned r;
    unsigned g;
    unsigned b;
    char    temp[FILE_MAX_PATH+1];
    char    temp_fn[FILE_MAX_PATH];
    std::strcpy(temp, g_map_name.c_str());
    std::strcpy(temp_fn, fn);
    merge_path_names(temp, temp_fn, CmdFile::AT_CMD_LINE);
    if (has_ext(temp) == nullptr)   // Did name have an extension?
    {
        std::strcat(temp, ".map");  // No? Then add .map
    }
    std::FILE *f = std::fopen(find_path(temp).c_str(), "r"); // search the dos path
    if (f == nullptr)
    {
        char line[160];
        std::snprintf(line, std::size(line), "Could not load color map %s", fn);
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
        dac[index].red   = (Byte)((r%256) >> 2);// maps default to 8 bits
        dac[index].green = (Byte)((g%256) >> 2);// DAC wants 6 bits
        dac[index].blue  = (Byte)((b%256) >> 2);
    }
    std::fclose(f);
    while (index < 256)
    {
        // zap unset entries
        dac[index].green = 40;
        dac[index].blue = 40;
        dac[index].red = 40;
        ++index;
    }
    g_color_state = ColorState::MAP_FILE;
    g_color_file = fn;
    return false;
}

//*************************************************************************

void set_color_palette_name(char const *fn)
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
