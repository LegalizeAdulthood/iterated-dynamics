// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "id_data.h"
#include "rotate.h"
#include "spindac.h"
#include "text_screen.h"
#include "unixscr.h"
#include "video.h"
#include "video_mode.h"
#include "zoom.h"

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
