// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/loadmap.h"

#include "engine/color_state.h"
#include "engine/color_utils.h"
#include "engine/spindac.h"
#include "io/library.h"
#include "misc/version.h"
#include "ui/stop_msg.h"

#include <config/port.h>

#include <fmt/format.h>

#include <array> // std::size
#include <cstdio>
#include <cstring>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

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

std::string g_last_map_name; // from last <l> <s> or colors=@filename
Byte g_map_clut[256][3]{};   // map= (default colors)
bool g_map_specified{};      // map= specified

static bool map_is_6bit_quantized()
{
    for (const auto &color : g_dac_box)
    {
        for (const Byte component : color)
        {
            if ((component & 0x03) != 0)
            {
                return false;
            }
        }
    }
    return true;
}

static void backwards_id1_3_map()
{
    const Version first_8bit_maps{1, 3, 1, 0, false};
    if (!(g_version < first_8bit_maps))
    {
        return;
    }
    if (!map_is_6bit_quantized())
    {
        return;
    }

    for (auto &color : g_dac_box)
    {
        for (Byte &component : color)
        {
            component = expand_8bit_color(component);
        }
    }
}

bool validate_luts(const std::string &map_name)
{
    std::filesystem::path map_path{map_name};
    if (!map_path.has_extension())
    {
        map_path.replace_extension(".map");
    }
    map_path = find_file(ReadFile::MAP, map_path);
    std::FILE *f = std::fopen(map_path.string().c_str(), "r"); // search the dos path
    if (f == nullptr)
    {
        stop_msg(fmt::format("Could not load color map {:s}", map_name));
        return true;
    }
    int index;
    for (index = 0; index < 256; index++)
    {
        char line[160];
        if (std::fgets(line, std::size(line), f) == nullptr)
        {
            break;
        }
        unsigned int r{};
        unsigned int g{};
        unsigned int b{};
        if (int count = std::sscanf(line, "%u %u %u", &r, &g, &b); count != 3)
        {
            stop_msg(fmt::format("Malformed map entry on line {:d}, only {:d} channels but expected 3.\n"
                                 "Skipping remaining lines.",
                index, count));
            break;
        }
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
    backwards_id1_3_map();
    g_color_state = ColorState::MAP_FILE;
    g_last_map_name = map_name;
    return false;
}

//*************************************************************************

void set_color_palette_name(const std::string &map_name)
{
    if (validate_luts(map_name))
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

} // namespace id::io
