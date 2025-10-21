// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <filesystem>

namespace id::io
{

extern Byte                  g_block[];
extern std::filesystem::path g_save_filename;   // save files using this name

int save_image(std::filesystem::path &filename);
bool encoder();

} // namespace id::io
