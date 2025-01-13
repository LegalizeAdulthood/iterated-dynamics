// SPDX-License-Identifier: GPL-3.0-only
//

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "misc/drivers.h"
#include "ui/cmdfiles.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/text_screen.h"
#include "ui/video.h"
#include "ui/video_mode.h"
#include "ui/zoom.h"
#include "unixscr.h"

#include <config/port.h>

#include <cstdio>
#include <cstring>

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

bool g_fake_lut{};

VideoInfo x11_video_table[] = {
    {999, 800, 600, 256, nullptr, "                         "},
};

void load_dac()
{
    read_video_palette();
}
