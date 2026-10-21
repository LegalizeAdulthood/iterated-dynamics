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
#include <cstdio>
#include <stdexcept>
#include <string>

namespace
{

void check_image(id::GIFInputFile &image, const id::fractals::FractalType type, const std::string &name,
    const id::engine::CalcStatus status, const int iterations)
{
    image.slurp();
    GifFileType *gif{static_cast<GifFileType *>(image)};
    const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
    const id::io::FormulaInfo definition{id::io::get_formula_info(gif)};
    if (image.num_images() != 1 || info.fractal_type != static_cast<int>(type) ||
        info.calc_status != static_cast<int>(status) || info.iterations != iterations ||
        std::string{definition.form_name} != name || !id::io::get_resume_info(gif).empty())
    {
        throw std::runtime_error{
            fmt::format("Unexpected IFS checkpoint: type={}/{}, status={}/{}, iterations={}/{}, name={}/{}, resume={}",
                static_cast<int>(info.fractal_type), static_cast<int>(type), static_cast<int>(info.calc_status),
                static_cast<int>(status), static_cast<int>(info.iterations), iterations, definition.form_name, name,
                id::io::get_resume_info(gif).size())};
    }
    const SavedImage &raster{image.get_image(0)};
    const GifByteType *begin{raster.RasterBits};
    const GifByteType *end{begin + raster.ImageDesc.Width * raster.ImageDesc.Height};
    if (std::all_of(begin, end, [](GifByteType color) { return color == 0; }))
    {
        throw std::runtime_error{"IFS checkpoint contains no plotted points"};
    }
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 6)
    {
        fmt::print(stderr, "Usage: check-ifs-interrupt type name interrupted restarted control\n");
        return 1;
    }
    try
    {
        const std::string type_name{argv[1]};
        if (type_name != "ifs" && type_name != "ifs3d")
        {
            throw std::runtime_error{"Unknown IFS type"};
        }
        // The ifs3d command is an alias; the definition header selects dimensions.
        const id::fractals::FractalType type{id::fractals::FractalType::IFS};
        id::GIFInputFile interrupted{argv[3]};
        id::GIFInputFile restarted{argv[4]};
        id::GIFInputFile control{argv[5]};
        check_image(interrupted, type, argv[2], id::engine::CalcStatus::NON_RESUMABLE, 64);
        check_image(restarted, type, argv[2], id::engine::CalcStatus::COMPLETED, 65);
        check_image(control, type, argv[2], id::engine::CalcStatus::COMPLETED, 65);
        const SavedImage &actual{restarted.get_image(0)};
        const SavedImage &expected{control.get_image(0)};
        if (actual.ImageDesc != expected.ImageDesc || restarted.color_map() != control.color_map() ||
            !std::equal(actual.RasterBits, actual.RasterBits + actual.ImageDesc.Width * actual.ImageDesc.Height,
                expected.RasterBits))
        {
            throw std::runtime_error{"Restarted IFS pixels differ from uninterrupted calculation"};
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
