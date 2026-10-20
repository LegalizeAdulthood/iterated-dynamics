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

int main(const int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: check-standard-resume checkpoint resumed control\n";
        return 1;
    }
    try
    {
        id::GIFInputFile checkpoint{argv[1]};
        checkpoint.slurp();
        GifFileType *checkpoint_gif{static_cast<GifFileType *>(checkpoint)};
        const id::io::FractalInfo info{id::io::get_fractal_info(checkpoint_gif)};
        if (info.calc_status != static_cast<int>(id::engine::CalcStatus::RESUMABLE) ||
            id::io::get_resume_info(checkpoint_gif).empty())
        {
            throw std::runtime_error{"Checkpoint did not interrupt resumable calculation"};
        }
        id::GIFInputFile resumed{argv[2]};
        id::GIFInputFile control{argv[3]};
        resumed.slurp();
        control.slurp();
        if (id::io::get_fractal_info(static_cast<GifFileType *>(resumed)).calc_status !=
                static_cast<int>(id::engine::CalcStatus::COMPLETED) ||
            id::io::get_fractal_info(static_cast<GifFileType *>(control)).calc_status !=
                static_cast<int>(id::engine::CalcStatus::COMPLETED))
        {
            throw std::runtime_error{"Final images are not complete"};
        }
        if (resumed.num_images() != 1 || control.num_images() != 1 || resumed.color_map() != control.color_map())
        {
            throw std::runtime_error{"Image count or palette differs"};
        }
        const SavedImage &actual{resumed.get_image(0)};
        const SavedImage &expected{control.get_image(0)};
        if (actual.ImageDesc != expected.ImageDesc ||
            !std::equal(actual.RasterBits, actual.RasterBits + actual.ImageDesc.Width * actual.ImageDesc.Height,
                expected.RasterBits))
        {
            throw std::runtime_error{"Resumed pixels differ from uninterrupted calculation"};
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
