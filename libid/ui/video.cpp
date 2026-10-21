// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/video.h"

#include "engine/pixel.h"
#include "io/disk_storage.h"
#include "misc/Driver.h"

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

int g_vesa_x_res{};
int g_vesa_y_res{};
int g_video_start_x{};
int g_video_start_y{};

void set_normal_dot()
{
    set_pixel_functions(driver_write_pixel, driver_read_pixel);
}

void set_disk_dot()
{
    set_pixel_functions(write_disk_pixel, read_disk_pixel);
}

static void null_write_pixel(int, int, int)
{
}

static int null_read_pixel(int, int)
{
    return 0;
}

void set_null_video()
{
    set_pixel_functions(null_write_pixel, null_read_pixel);
}

} // namespace id::ui
