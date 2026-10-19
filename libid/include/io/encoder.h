// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <cstdio>
#include <filesystem>

namespace id::io
{

struct FractalInfo;
class GifWriter;

extern Byte                  g_block[];
extern std::filesystem::path g_save_filename;   // save files using this name

int save_image(std::filesystem::path &filename);
// Returns true after all pixels and metadata are written and output is closed.
bool encoder(GifWriter &writer, const std::filesystem::path &filename);
void setup_save_info(FractalInfo *save_info);
bool write_gif_uint16(std::FILE *file, int value);

} // namespace id::io
