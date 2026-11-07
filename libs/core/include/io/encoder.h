// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <filesystem>

namespace id::io
{

struct FractalInfo;

extern std::filesystem::path g_save_filename;   // save files using this name

void setup_save_info(FractalInfo *save_info);

} // namespace id::io
