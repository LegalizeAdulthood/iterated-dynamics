// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <FindFractalInfoTestSupport.h>

#include <io/loadfile.h>

namespace id::test
{

bool FindFractalInfoResult::read(const std::filesystem::path &path, const bool apply_palette)
{
    return io::find_fractal_info(
        path.string(), &info, &resume, &formula, &ranges, &parameters, &evolution, &orbits, apply_palette);
}

} // namespace id::test
