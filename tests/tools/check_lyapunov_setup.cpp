// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>

#include <image-tool/GIFInputFile.h>
#include <image-tool/gif_compare.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 4)
    {
        fmt::print(stderr, "Usage: check-lyapunov-setup normalized corrected control\n");
        return 1;
    }
    try
    {
        for (int checkpoint = 1; checkpoint <= 3; ++checkpoint)
        {
            if (!std::filesystem::is_regular_file(argv[checkpoint]))
            {
                throw std::runtime_error{fmt::format("Missing checkpoint: {}", argv[checkpoint])};
            }
            id::GIFInputFile image{argv[checkpoint]};
            image.slurp();
            GifFileType *gif{static_cast<GifFileType *>(image)};
            const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
            if (info.fractal_type != static_cast<int>(id::fractals::FractalType::LYAPUNOV) ||
                info.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED) ||
                info.inside != (checkpoint == 1 ? 1 : 2) || info.std_calc_mode != '1' ||
                !id::io::get_resume_info(gif).empty())
            {
                throw std::runtime_error{"Unexpected Lyapunov type, completion, inside color, or pass"};
            }
        }
        id::GIFInputFile corrected{argv[2]};
        id::GIFInputFile control{argv[3]};
        corrected.slurp();
        control.slurp();
        const SavedImage &actual{corrected.get_image(0)};
        const SavedImage &expected{control.get_image(0)};
        if (actual.ImageDesc != expected.ImageDesc || corrected.color_map() != control.color_map() ||
            !std::equal(expected.RasterBits, expected.RasterBits + expected.ImageDesc.Width * expected.ImageDesc.Height,
                actual.RasterBits))
        {
            throw std::runtime_error{"Corrected Lyapunov pixels differ from a fresh calculation"};
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
