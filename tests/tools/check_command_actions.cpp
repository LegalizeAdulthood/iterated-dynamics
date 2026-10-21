// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/loadfile.h>

#include <fmt/format.h>

#include <exception>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        return 2;
    }
    try
    {
        const id::io::ImageMetadata accepted{id::io::read_image_metadata(argv[1])};
        const id::io::ImageMetadata cancelled{id::io::read_image_metadata(argv[2])};
        if (accepted.fractal.fractal_type != static_cast<int>(id::fractals::FractalType::FORMULA) ||
            accepted.fractal.iterations != 321 ||
            cancelled.fractal.fractal_type != static_cast<int>(id::fractals::FractalType::MANDEL) ||
            cancelled.fractal.iterations != 150 ||
            accepted.fractal.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED) ||
            cancelled.fractal.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED))
        {
            throw std::runtime_error("Startup selection did not accept the formula or restore defaults on cancel");
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
