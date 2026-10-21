// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>

#include <image-tool/gif_compare.h>
#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        fmt::print(stderr, "Usage: check-diffusion-resume checkpoint completed\n");
        return 1;
    }
    try
    {
        id::GIFInputFile checkpoint{argv[1]};
        id::GIFInputFile completed{argv[2]};
        checkpoint.slurp();
        completed.slurp();
        GifFileType *before{static_cast<GifFileType *>(checkpoint)};
        GifFileType *after{static_cast<GifFileType *>(completed)};
        const id::io::FractalInfo initial{id::io::get_fractal_info(before)};
        const id::io::FractalInfo result{id::io::get_fractal_info(after)};
        if (initial.fractal_type != static_cast<int>(id::fractals::FractalType::DIFFUSION) ||
            result.fractal_type != initial.fractal_type ||
            initial.calc_status != static_cast<int>(id::engine::CalcStatus::RESUMABLE) ||
            id::io::get_resume_info(before).empty() ||
            result.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED))
        {
            throw std::runtime_error("Diffusion did not interrupt, save resume data, and complete");
        }
        if (checkpoint.num_images() != 1 || completed.num_images() != 1 ||
            checkpoint.color_map() != completed.color_map())
        {
            throw std::runtime_error("Image count or palette changed during continuation");
        }
        const SavedImage &initial_image{checkpoint.get_image(0)};
        const SavedImage &final_image{completed.get_image(0)};
        if (initial_image.ImageDesc != final_image.ImageDesc)
        {
            throw std::runtime_error("Image dimensions changed during continuation");
        }
        bool progressed{};
        for (int i = 0; i < initial_image.ImageDesc.Width * initial_image.ImageDesc.Height; ++i)
        {
            if (initial_image.RasterBits[i] != 0 && initial_image.RasterBits[i] != final_image.RasterBits[i])
            {
                throw std::runtime_error("Continuation changed an existing cluster pixel");
            }
            progressed = progressed || (initial_image.RasterBits[i] == 0 && final_image.RasterBits[i] != 0);
        }
        if (!progressed)
        {
            throw std::runtime_error("Continuation did not grow the cluster");
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
