// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "io/decoder.h"

#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/VideoInfo.h"
#include "misc/Driver.h"

using namespace id::engine;
using namespace id::misc;

namespace id::io
{

OutLine g_out_line{out_line};
int g_row_count{};

int out_line(Byte *pixels, const int line_len)
{
    driver_check_memory();
    if (g_row_count + g_logical_screen.y_offset >= g_screen_y_dots)
    {
        return 0;
    }
    write_span(g_row_count, 0, line_len - 1, pixels);
    ++g_row_count;
    return 0;
}

} // namespace id::io
