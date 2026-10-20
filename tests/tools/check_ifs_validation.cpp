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
#include <string>

int main(const int argc, char *argv[])
{
    if (argc != 4)
    {
        fmt::print(stderr, "Usage: check-ifs-validation rejected corrected cancelled\n");
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
            const id::io::FormulaInfo definition{id::io::get_formula_info(gif)};
            const id::engine::CalcStatus status{
                checkpoint == 1 ? id::engine::CalcStatus::NON_RESUMABLE : id::engine::CalcStatus::COMPLETED};
            if (info.fractal_type != static_cast<int>(id::fractals::FractalType::IFS) ||
                info.calc_status != static_cast<int>(status) ||
                std::string{definition.form_name} != (checkpoint == 1 ? "missing" : "good"))
            {
                throw std::runtime_error{"Unexpected IFS selection or calculation status"};
            }
            if (checkpoint == 2)
            {
                const SavedImage &raster{image.get_image(0)};
                const int pixels{raster.ImageDesc.Width * raster.ImageDesc.Height};
                if (std::all_of(raster.RasterBits, raster.RasterBits + pixels,
                        [&raster](GifByteType color) { return color == raster.RasterBits[0]; }))
                {
                    throw std::runtime_error{"Corrected IFS image has no color variation"};
                }
            }
        }
        id::GIFInputFile corrected{argv[2]};
        id::GIFInputFile cancelled{argv[3]};
        corrected.slurp();
        cancelled.slurp();
        const SavedImage &expected{corrected.get_image(0)};
        const SavedImage &actual{cancelled.get_image(0)};
        if (expected.ImageDesc != actual.ImageDesc || corrected.color_map() != cancelled.color_map() ||
            !std::equal(expected.RasterBits, expected.RasterBits + expected.ImageDesc.Width * expected.ImageDesc.Height,
                actual.RasterBits))
        {
            throw std::runtime_error{"Cancelling invalid IFS selection changed the completed image"};
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
