#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "find_path.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"

#include <cstdio>
#include <cstring>

//*************************************************************************

namespace
{

struct Palettetype
{
    BYTE red;
    BYTE green;
    BYTE blue;
};

} // namespace

#define dac ((Palettetype *)g_dac_box)

bool ValidateLuts(char const *fn)
{
    unsigned        r, g, b;
    char    temp[FILE_MAX_PATH+1];
    char    temp_fn[FILE_MAX_PATH];
    std::strcpy(temp, g_map_name.c_str());
    std::strcpy(temp_fn, fn);
#ifdef XFRACT
    merge_pathnames(temp, temp_fn, cmd_file::AT_CMD_LINE_SET_NAME);
#else
    merge_pathnames(temp, temp_fn, cmd_file::AT_CMD_LINE);
#endif
    if (has_ext(temp) == nullptr)   // Did name have an extension?
    {
        std::strcat(temp, ".map");  // No? Then add .map
    }
    std::FILE *f = std::fopen(find_path(temp).c_str(), "r"); // search the dos path
    if (f == nullptr)
    {
        char line[160];
        std::snprintf(line, NUM_OF(line), "Could not load color map %s", fn);
        stopmsg(STOPMSG_NONE, line);
        return true;
    }
    unsigned index;
    for (index = 0; index < 256; index++)
    {
        char line[160];
        if (std::fgets(line, NUM_OF(line), f) == nullptr)
        {
            break;
        }
        std::sscanf(line, "%u %u %u", &r, &g, &b);
        //* load global dac values *
        dac[index].red   = (BYTE)((r%256) >> 2);// maps default to 8 bits
        dac[index].green = (BYTE)((g%256) >> 2);// DAC wants 6 bits
        dac[index].blue  = (BYTE)((b%256) >> 2);
    }
    std::fclose(f);
    while (index < 256)
    {
        // zap unset entries
        dac[index].green = 40;
        dac[index].blue = dac[index].green;
        dac[index].red = dac[index].blue;
        ++index;
    }
    g_color_state = 2;
    g_color_file = fn;
    return false;
}

//*************************************************************************

void SetColorPaletteName(char const *fn)
{
    if (ValidateLuts(fn))
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
