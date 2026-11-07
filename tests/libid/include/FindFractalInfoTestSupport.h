// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <io/gif_extensions.h>

#include <filesystem>

namespace id::test
{

struct FindFractalInfoResult
{
    io::FractalInfo info{};
    io::ExtBlock2 resume{};
    io::ExtBlock3 formula{};
    io::ExtBlock4 ranges{};
    io::ExtBlock5 parameters{};
    io::ExtBlock6 evolution{};
    io::ExtBlock7 orbits{};

    // Returns true on error, matching find_fractal_info.
    bool read(const std::filesystem::path &path, bool apply_palette = false);
};

} // namespace id::test
