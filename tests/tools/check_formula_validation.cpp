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
        fmt::print(stderr, "Usage: check-formula-validation rejected corrected cancelled\n");
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
            if (info.fractal_type != static_cast<int>(id::fractals::FractalType::FORMULA) ||
                info.calc_status != static_cast<int>(status) || !id::io::get_resume_info(gif).empty() ||
                std::string{definition.form_name} != (checkpoint == 1 ? "missing" : "good"))
            {
                throw std::runtime_error{"Unexpected formula selection or calculation status"};
            }
            const SavedImage &raster{image.get_image(0)};
            const int pixels{raster.ImageDesc.Width * raster.ImageDesc.Height};
            const bool blank{std::all_of(
                raster.RasterBits, raster.RasterBits + pixels, [](GifByteType color) { return color == 0; })};
            if (blank != (checkpoint == 1))
            {
                throw std::runtime_error{"Expected blank rejected formula and nonblank completed image"};
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
            throw std::runtime_error{"Cancelling invalid formula selection changed the completed image"};
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
