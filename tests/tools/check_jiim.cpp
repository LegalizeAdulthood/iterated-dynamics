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
#include <string_view>

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

void check_same_image(id::GIFInputFile &initial, id::GIFInputFile &restored)
{
    const SavedImage &before{initial.get_image(0)};
    const SavedImage &after{restored.get_image(0)};
    if (before.ImageDesc != after.ImageDesc || initial.color_map() != restored.color_map() ||
        !std::equal(
            before.RasterBits, before.RasterBits + before.ImageDesc.Width * before.ImageDesc.Height, after.RasterBits))
    {
        throw std::runtime_error{"Picker/orbit window did not restore the Mandelbrot image"};
    }
}

void check_viewport(char *argv[])
{
    id::GIFInputFile initial{argv[2]};
    id::GIFInputFile restored{argv[3]};
    id::GIFInputFile captured{argv[4]};
    id::GIFInputFile restarted{argv[5]};
    id::GIFInputFile control{argv[6]};
    for (id::GIFInputFile *image : {&initial, &restored, &captured, &restarted, &control})
    {
        check_image(*image, id::fractals::FractalType::MANDEL);
    }
    const GifImageDesc &small{initial.get_image(0).ImageDesc};
    const GifImageDesc &full{captured.get_image(0).ImageDesc};
    if (small.Width != 8 || small.Height != 6 || full.Width <= small.Width || full.Height <= small.Height)
    {
        throw std::runtime_error{"Orbit save did not promote the reduced viewport to the full screen"};
    }
    const id::io::FractalInfo info{id::io::get_fractal_info(static_cast<GifFileType *>(restarted))};
    if (info.iterations != 151)
    {
        throw std::runtime_error{"New parameters did not restart the reduced-viewport calculation"};
    }
    check_same_image(initial, restored);
    check_same_image(restarted, control);
}

} // namespace

int main(const int argc, char *argv[])
{
    const bool viewport{argc == 7 && std::string_view{argv[1]} == "--viewport"};
    if (argc != 5 && !viewport)
    {
        std::cerr << "Usage: check-jiim initial moved exact restored\n"
                     "       check-jiim --viewport initial restored captured restarted control\n";
        return 1;
    }
    try
    {
        if (viewport)
        {
            check_viewport(argv);
            return 0;
        }
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
        check_same_image(initial, restored);
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
