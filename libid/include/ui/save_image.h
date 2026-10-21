// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <filesystem>

namespace id::io
{
class ImageSave;
}

namespace id::ui
{

int save_image(std::filesystem::path &filename);
// Complete the save and scanline feedback without polling or dispatching input.
bool write_image(io::ImageSave &image);

} // namespace id::ui
