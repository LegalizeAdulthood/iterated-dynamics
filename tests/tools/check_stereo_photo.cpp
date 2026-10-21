// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <fractals/fractype.h>
#include <geometry/plot3d.h>
#include <io/gif_file.h>

#include <image-tool/GIFInputFile.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace
{

void check_image(const char *path, const id::fractals::FractalType type, const id::engine::CalcStatus status)
{
    if (!std::filesystem::is_regular_file(path))
    {
        throw std::runtime_error{fmt::format("Missing stereo checkpoint {}", path)};
    }
    id::GIFInputFile image{path};
    image.slurp();
    GifFileType *gif{static_cast<GifFileType *>(image)};
    const id::io::FractalInfo info{id::io::get_fractal_info(gif)};
    if (image.num_images() != 1 || info.fractal_type != static_cast<int>(type) ||
        info.calc_status != static_cast<int>(status) || info.iterations != 64 ||
        info.glasses_type != static_cast<int>(id::geometry::GlassesType::PHOTO) ||
        !id::io::get_resume_info(gif).empty())
    {
        throw std::runtime_error{fmt::format("Unexpected stereo checkpoint {}: type={}/{}, status={}/{}, "
                                             "iterations={}, glasses={}, resume={}",
            path, static_cast<int>(info.fractal_type), static_cast<int>(type), static_cast<int>(info.calc_status),
            static_cast<int>(status), static_cast<int>(info.iterations), static_cast<int>(info.glasses_type),
            id::io::get_resume_info(gif).size())};
    }
    const SavedImage &raster{image.get_image(0)};
    const GifByteType *begin{raster.RasterBits};
    const GifByteType *end{begin + raster.ImageDesc.Width * raster.ImageDesc.Height};
    if (raster.ImageDesc.Width != 64 || raster.ImageDesc.Height != 48 ||
        std::all_of(begin, end, [](GifByteType color) { return color == 0; }))
    {
        throw std::runtime_error{fmt::format("Missing stereo image output in {}", path)};
    }
    if (type == id::fractals::FractalType::IFS && std::string{id::io::get_formula_info(gif).form_name} != "3dfern")
    {
        throw std::runtime_error{"Unexpected IFS definition in stereo checkpoint"};
    }
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 5)
    {
        fmt::print(stderr, "Usage: check-stereo-photo type complete|cancel left right\n");
        return 1;
    }
    try
    {
        const std::string type_name{argv[1]};
        const std::string result{argv[2]};
        if ((type_name != "ifs" && type_name != "lorenz3d") || (result != "complete" && result != "cancel"))
        {
            throw std::runtime_error{"Unknown stereo photographer test case"};
        }
        const id::fractals::FractalType type{
            type_name == "ifs" ? id::fractals::FractalType::IFS : id::fractals::FractalType::LORENZ_3D};
        check_image(argv[3], type, id::engine::CalcStatus::IN_PROGRESS);
        check_image(argv[4], type,
            result == "complete" ? id::engine::CalcStatus::COMPLETED : id::engine::CalcStatus::NON_RESUMABLE);
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
