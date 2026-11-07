// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <gif_lib.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace id::test
{

using GifFilePtr = std::unique_ptr<GifFileType, void (*)(GifFileType *)>;

GifFilePtr read_gif(const std::filesystem::path &path);
void expect_gif_raster(const GifFileType &gif, const std::vector<Byte> &pixels, int width, int height);

} // namespace id::test
