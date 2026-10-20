// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>

#include <image-tool/gif_compare.h>
#include <image-tool/GIFInputFile.h>

#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: check-cellular-preparation interrupted restarted control\n";
        return 1;
    }
    try
    {
        id::GIFInputFile interrupted{argv[1]};
        interrupted.slurp();
        GifFileType *checkpoint{static_cast<GifFileType *>(interrupted)};
        const id::io::FractalInfo initial{id::io::get_fractal_info(checkpoint)};
        if (initial.fractal_type != static_cast<int>(id::fractals::FractalType::CELLULAR) ||
            initial.calc_status != static_cast<int>(id::engine::CalcStatus::NON_RESUMABLE) ||
            initial.d_param4 != 100000000.0 || !id::io::get_resume_info(checkpoint).empty())
        {
            throw std::runtime_error{
                fmt::format("Invalid preparation checkpoint: type={}, status={}, start={}, resume={}",
                    static_cast<int>(initial.fractal_type), static_cast<int>(initial.calc_status),
                    static_cast<double>(initial.d_param4), id::io::get_resume_info(checkpoint).size())};
        }
        id::GIFInputFile restarted{argv[2]};
        id::GIFInputFile control{argv[3]};
        restarted.slurp();
        control.slurp();
        for (id::GIFInputFile *image : {&restarted, &control})
        {
            const id::io::FractalInfo info{id::io::get_fractal_info(static_cast<GifFileType *>(*image))};
            if (info.fractal_type != static_cast<int>(id::fractals::FractalType::CELLULAR) ||
                info.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED) || info.d_param4 != 0.0)
            {
                throw std::runtime_error{"Expected completed cellular rendering restarted at generation zero"};
            }
        }
        if (restarted.num_images() != 1 || control.num_images() != 1 || restarted.color_map() != control.color_map())
        {
            throw std::runtime_error{"Image count or palette differs"};
        }
        const SavedImage &actual{restarted.get_image(0)};
        const SavedImage &expected{control.get_image(0)};
        if (actual.ImageDesc != expected.ImageDesc ||
            !std::equal(actual.RasterBits, actual.RasterBits + actual.ImageDesc.Width * actual.ImageDesc.Height,
                expected.RasterBits))
        {
            throw std::runtime_error{"Restarted pixels differ from a fresh cellular calculation"};
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
