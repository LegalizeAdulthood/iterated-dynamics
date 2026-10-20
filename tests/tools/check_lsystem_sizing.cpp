// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <io/gif_file.h>

#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{

void check_image(const char *filename, const int order, const id::engine::CalcStatus status)
{
    id::GIFInputFile image{filename};
    image.slurp();
    GifFileType *gif{static_cast<GifFileType *>(image)};
    const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
    const id::io::FormulaInfo definition{id::io::get_formula_info(gif)};
    if (info.fractal_type != static_cast<int>(id::fractals::FractalType::L_SYSTEM) || info.c_real != order ||
        info.calc_status != static_cast<int>(status) || std::string{definition.form_name} != "Koch1" ||
        !id::io::get_resume_info(gif).empty())
    {
        throw std::runtime_error{fmt::format("Invalid L-system checkpoint: type={}, order={}, status={}, name={}",
            static_cast<int>(info.fractal_type), static_cast<double>(info.c_real), static_cast<int>(info.calc_status),
            definition.form_name)};
    }
    if (image.num_images() != 1)
    {
        throw std::runtime_error{"Expected one L-system image"};
    }
    const SavedImage &frame{image.get_image(0)};
    const GifByteType *begin{frame.RasterBits};
    const GifByteType *end{begin + frame.ImageDesc.Width * frame.ImageDesc.Height};
    const bool blank{std::all_of(begin, end, [](GifByteType pixel) { return pixel == 0; })};
    if (blank != (status == id::engine::CalcStatus::NON_RESUMABLE))
    {
        throw std::runtime_error{"Expected blank interrupted sizing and nonblank completed drawing"};
    }
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: check-lsystem-sizing interrupted restarted\n";
        return 1;
    }
    try
    {
        check_image(argv[1], 9, id::engine::CalcStatus::NON_RESUMABLE);
        check_image(argv[2], 1, id::engine::CalcStatus::COMPLETED);
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
