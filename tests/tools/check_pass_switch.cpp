// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <io/gif_file.h>

#include <image-tool/gif_compare.h>
#include <image-tool/GIFInputFile.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{

void check_image(const char *actual_path, const char *control_path, const char pass)
{
    id::GIFInputFile actual{actual_path};
    id::GIFInputFile control{control_path};
    actual.slurp();
    control.slurp();
    for (id::GIFInputFile *image : {&actual, &control})
    {
        const id::io::FractalInfo info{id::io::get_fractal_info(static_cast<GifFileType *>(*image))};
        if (info.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED) || info.std_calc_mode != pass ||
            info.bf_math == 0)
        {
            throw std::runtime_error{"Expected a completed arbitrary-precision image with the selected pass"};
        }
    }
    if (actual.num_images() != 1 || control.num_images() != 1 || actual.color_map() != control.color_map())
    {
        throw std::runtime_error{"Image count or palette differs"};
    }
    const SavedImage &rendered{actual.get_image(0)};
    const SavedImage &expected{control.get_image(0)};
    const int pixel_count{rendered.ImageDesc.Width * rendered.ImageDesc.Height};
    if (rendered.ImageDesc != expected.ImageDesc ||
        !std::equal(rendered.RasterBits, rendered.RasterBits + pixel_count, expected.RasterBits))
    {
        throw std::runtime_error{"Switched pixels differ from a fresh calculation"};
    }
    if (std::all_of(rendered.RasterBits, rendered.RasterBits + pixel_count,
            [&rendered](const GifByteType pixel) { return pixel == rendered.RasterBits[0]; }))
    {
        throw std::runtime_error{"Pass-switch image has no color variation"};
    }
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 7)
    {
        std::cerr << "Usage: check-pass-switch initial-b switched-g switched-b repeated-g control-b control-g\n";
        return 1;
    }
    try
    {
        check_image(argv[1], argv[5], 'b');
        check_image(argv[2], argv[6], 'g');
        check_image(argv[3], argv[5], 'b');
        check_image(argv[4], argv[6], 'g');
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
