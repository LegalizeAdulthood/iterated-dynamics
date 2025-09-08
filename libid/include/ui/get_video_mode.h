// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::io
{
struct ExtBlock3;
struct FractalInfo;
} // namespace id::io

namespace id::ui
{

int get_video_mode(io::FractalInfo *info, io::ExtBlock3 *blk_3_info);

} // namespace id::ui
