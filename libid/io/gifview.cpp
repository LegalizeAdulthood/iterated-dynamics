// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// GIF scanline pixel output helpers.
#include "io/decoder.h"
#include "io/gifview.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/Potential.h"
#include "engine/spindac.h"
#include "engine/VideoInfo.h"
#include "io/disk_storage.h"
#include "misc/Driver.h"
#include "ui/diskvid.h"

#include <cstdlib>
#include <algorithm>
#include <vector>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

bool g_dither_flag{};                           // true if we want to dither GIFs
unsigned int g_height{};
unsigned int g_num_colors{};

static std::vector<char> s_dither_buf;

// routine for MIGS that generates partial output lines

int out_line_migs(Byte *pixels, const int line_len, const int left, const int top)
{
    const int row = top + g_row_count;
    const int start_col = left;
    const int stop_col = start_col + line_len - 1;
    write_span(row, start_col, stop_col, pixels);
    g_row_count++;

    return 0;
}

int out_line_dither(Byte *pixels, const int line_len)
{
    int err;
    s_dither_buf.resize(line_len + 1);
    std::fill(s_dither_buf.begin(), s_dither_buf.end(), 0);

    // Viewing-only dither noise; do not consume the image RNG stream.
    int next_err = (std::rand() & 0x1f) - 16;
    for (int i = 0; i < line_len; i++)
    {
        // TODO: 6-bit color
        int brt = (g_dac_box[pixels[i]][0] * 5 + g_dac_box[pixels[i]][1] * 9 + g_dac_box[pixels[i]][2] * 2) >>
            4; // brightness from 0 to 63
        brt += next_err;
        if (brt > 32)
        {
            pixels[i] = 1;
            err = brt-63;
        }
        else
        {
            pixels[i] = 0;
            err = brt;
        }
        next_err = s_dither_buf[i+1]+err/3;
        s_dither_buf[i] = static_cast<char>(err / 3);
        s_dither_buf[i+1] = static_cast<char>(err / 3);
    }
    return out_line(pixels, line_len);
}

int pot_line(Byte *pixels, const int line_len)
{
    if (g_row_count == 0)
    {
        if (pot_start_disk() < 0)
        {
            return -1;
        }
    }
    int save_row_count = g_row_count;
    g_row_count >>= 1;
    int row = g_row_count;
    if ((save_row_count & 1) != 0)   // odd line
    {
        row += g_logical_screen.y_dots;
    }
    else if (!driver_is_disk())     // even line - display the line too
    {
        out_line(pixels, line_len);
    }
    for (int col = 0; col < g_logical_screen.x_dots; ++col)
    {
        write_disk_pixel(col+g_logical_screen.x_offset, row+g_logical_screen.y_offset, *(pixels+col));
    }
    g_row_count = save_row_count + 1;
    return 0;
}

} // namespace id::io
