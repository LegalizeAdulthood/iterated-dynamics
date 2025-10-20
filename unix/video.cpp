// SPDX-License-Identifier: GPL-3.0-only
//
#include "video.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/VideoInfo.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/text_screen.h"
#include "ui/video.h"
#include "ui/zoom.h"

#include "general.h"
#include "unixscr.h"

#include <config/port.h>

#include <cstdio>
#include <cstring>

using namespace id::engine;

namespace id::misc
{

/*
 * This file contains Unix versions of the routines in video.asm
 * Copyright 1992 Ken Shirriff
 */

VideoInfo x11_video_table[] = {
    {999, 800, 600, 256, nullptr, "                         "},
};

void load_dac()
{
    read_video_palette();
}

} // namespace id::misc
