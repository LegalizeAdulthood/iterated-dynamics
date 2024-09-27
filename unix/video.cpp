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

VIDEOINFO x11_video_table[] = {
    {999, 800, 600, 256, nullptr, "                         "},
};

void loaddac()
{
    readvideopalette();
}
