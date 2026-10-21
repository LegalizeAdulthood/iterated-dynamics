// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/loadfile.h>

#include <fmt/format.h>

#include <exception>
#include <stdexcept>

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        return 2;
    }
    try
    {
        const id::io::ImageMetadata before{id::io::read_image_metadata(argv[1])};
        const id::io::ImageMetadata after{id::io::read_image_metadata(argv[2])};
        if (before.fractal.iterations != 150 || before.fractal != after.fractal)
        {
            throw std::runtime_error("Cancelled restore changed active image parameters");
        }
    }
    catch (const std::exception &error)
    {
        fmt::print(stderr, "{}\n", error.what());
        return 1;
    }
    return 0;
}
