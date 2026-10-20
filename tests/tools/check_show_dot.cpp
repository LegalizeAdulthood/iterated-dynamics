// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <io/gif_file.h>

#include <image-tool/gif_compare.h>
#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace
{

int check_checkpoint(const char *path, id::GIFInputFile &control)
{
    id::GIFInputFile checkpoint{path};
    checkpoint.slurp();
    GifFileType *gif{static_cast<GifFileType *>(checkpoint)};
    const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
    if (info.calc_status != static_cast<int>(id::engine::CalcStatus::RESUMABLE) || info.iterations != 150 ||
        id::io::get_resume_info(gif).empty())
    {
        throw std::runtime_error{"Expected an interrupted, resumable 150-iteration checkpoint"};
    }
    const SavedImage &actual{checkpoint.get_image(0)};
    const SavedImage &expected{control.get_image(0)};
    if (actual.ImageDesc != expected.ImageDesc || checkpoint.color_map() != control.color_map())
    {
        throw std::runtime_error{"Checkpoint geometry or palette differs"};
    }
    if (!std::equal(actual.RasterBits, actual.RasterBits + actual.ImageDesc.Width, expected.RasterBits))
    {
        throw std::runtime_error{"Show-dot interruption happened before completing the first row"};
    }
    int computed{};
    const int count{actual.ImageDesc.Width * actual.ImageDesc.Height};
    for (int pixel = 0; pixel < count; ++pixel)
    {
        if (actual.RasterBits[pixel] != 0)
        {
            ++computed;
            if (actual.RasterBits[pixel] != expected.RasterBits[pixel])
            {
                throw std::runtime_error{"Marker pixels contaminated the interrupted image"};
            }
        }
    }
    return computed;
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 6)
    {
        fmt::print(stderr, "Usage: check-show-dot initial resumed restarted control150 control300\n");
        return 1;
    }
    try
    {
        id::GIFInputFile initial_control{argv[4]};
        initial_control.slurp();
        const int initial{check_checkpoint(argv[1], initial_control)};
        const int resumed{check_checkpoint(argv[2], initial_control)};
        if (resumed <= initial)
        {
            throw std::runtime_error{"Escaping the options screen did not continue calculation"};
        }
        id::GIFInputFile restarted{argv[3]};
        id::GIFInputFile final_control{argv[5]};
        restarted.slurp();
        final_control.slurp();
        const id::io::FractalInfo info{id::io::get_fractal_info(static_cast<GifFileType *>(restarted))};
        const SavedImage &actual{restarted.get_image(0)};
        const SavedImage &expected{final_control.get_image(0)};
        if (info.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED) || info.iterations != 300)
        {
            throw std::runtime_error{"Maximum Iterations did not restart and complete calculation"};
        }
        if (actual.ImageDesc != expected.ImageDesc || restarted.color_map() != final_control.color_map() ||
            !std::equal(actual.RasterBits, actual.RasterBits + actual.ImageDesc.Width * actual.ImageDesc.Height,
                expected.RasterBits))
        {
            throw std::runtime_error{"Restarted show-dot image differs from the control"};
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
