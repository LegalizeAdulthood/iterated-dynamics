// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::io
{
struct ExtBlock3;
struct FractalInfo;
}

int get_video_mode(id::io::FractalInfo *info, id::io::ExtBlock3 *blk_3_info);
