// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <io/EvolutionInfo.h>

#include <filesystem>
#include <optional>

namespace id::io
{
class ImageSave;
}

namespace id::ui
{

std::optional<io::EvolutionInfo> capture_evolution_info();
int save_image(std::filesystem::path &filename);
// Complete the save and scanline feedback without polling or dispatching input.
bool write_image(io::ImageSave &image);

} // namespace id::ui
