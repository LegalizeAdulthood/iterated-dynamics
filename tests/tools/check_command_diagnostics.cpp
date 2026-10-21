// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>
#include <io/loadfile.h>

#include <fmt/format.h>

#include <exception>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 2)
    {
        return 2;
    }
    try
    {
        const id::io::ImageMetadata metadata{id::io::read_image_metadata(argv[1])};
        if (metadata.fractal.iterations != 321 ||
            metadata.fractal.calc_status != static_cast<int>(id::engine::CalcStatus::COMPLETED))
        {
            throw std::runtime_error("Command retry did not complete with the requested iteration limit");
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
