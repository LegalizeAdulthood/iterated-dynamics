// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <io/gif_file.h>

#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        fmt::print(stderr, "Usage: check-standard-validation rejected corrected\n");
        return 1;
    }
    try
    {
        for (int checkpoint = 1; checkpoint <= 2; ++checkpoint)
        {
            if (!std::filesystem::is_regular_file(argv[checkpoint]))
            {
                throw std::runtime_error{fmt::format("Missing checkpoint: {}", argv[checkpoint])};
            }
            id::GIFInputFile image{argv[checkpoint]};
            image.slurp();
            GifFileType *gif{static_cast<GifFileType *>(image)};
            const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
            const id::engine::CalcStatus status{
                checkpoint == 1 ? id::engine::CalcStatus::NON_RESUMABLE : id::engine::CalcStatus::COMPLETED};
            if (info.std_calc_mode != 'b' || info.inside != checkpoint - 1 ||
                info.calc_status != static_cast<int>(status) || !id::io::get_resume_info(gif).empty())
            {
                throw std::runtime_error{"Unexpected validation checkpoint status, pass, or inside color"};
            }
            if (checkpoint == 2)
            {
                const SavedImage &raster{image.get_image(0)};
                const int pixels{raster.ImageDesc.Width * raster.ImageDesc.Height};
                if (std::all_of(raster.RasterBits, raster.RasterBits + pixels,
                        [&raster](GifByteType color) { return color == raster.RasterBits[0]; }))
                {
                    throw std::runtime_error{"Corrected image has no color variation"};
                }
            }
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
