// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>

#include <image-tool/gif_compare.h>
#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{

id::io::FractalInfo check_image(id::GIFInputFile &image, const id::fractals::FractalType type)
{
    image.slurp();
    const id::io::FractalInfo info{id::io::get_fractal_info(static_cast<GifFileType *>(image))};
    if (image.num_images() != 1 || info.fractal_type != static_cast<int>(type) ||
        info.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED))
    {
        throw std::runtime_error{"Expected one completed image of the selected fractal type"};
    }
    return info;
}

void check_point(const id::io::FractalInfo &info, const double x, const double y)
{
    if (std::abs(info.c_real - x) > 1e-12 || std::abs(info.c_imag - y) > 1e-12)
    {
        throw std::runtime_error{fmt::format("Selected point ({}, {}) differs from ({}, {})",
            static_cast<double>(info.c_real), static_cast<double>(info.c_imag), x, y)};
    }
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 5)
    {
        std::cerr << "Usage: check-jiim initial moved exact restored\n";
        return 1;
    }
    try
    {
        id::GIFInputFile initial{argv[1]};
        id::GIFInputFile moved{argv[2]};
        id::GIFInputFile exact{argv[3]};
        id::GIFInputFile restored{argv[4]};
        const id::io::FractalInfo start{check_image(initial, id::fractals::FractalType::MANDEL)};
        const id::io::FractalInfo selection{check_image(moved, id::fractals::FractalType::JULIA)};
        check_point(selection, start.x_min + (start.x_dots / 2 + 1) * (start.x_max - start.x_min) / (start.x_dots - 1),
            start.y_max - (start.y_dots / 2 + 1) * (start.y_max - start.y_min) / (start.y_dots - 1));
        check_point(check_image(exact, id::fractals::FractalType::JULIA), -0.75, 0.125);
        check_image(restored, id::fractals::FractalType::MANDEL);
        const SavedImage &before{initial.get_image(0)};
        const SavedImage &after{restored.get_image(0)};
        if (before.ImageDesc != after.ImageDesc || initial.color_map() != restored.color_map() ||
            !std::equal(before.RasterBits, before.RasterBits + before.ImageDesc.Width * before.ImageDesc.Height,
                after.RasterBits))
        {
            throw std::runtime_error{"Picker/orbit window did not restore the Mandelbrot image"};
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
