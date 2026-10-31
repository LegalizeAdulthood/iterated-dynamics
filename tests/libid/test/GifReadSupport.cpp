// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <GifReadSupport.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <string>

namespace id::test
{
namespace
{

void close_gif(GifFileType *input)
{
    int error{};
    DGifCloseFile(input, &error);
}

} // namespace

GifFilePtr read_gif(const std::filesystem::path &path)
{
    int error{};
    GifFilePtr gif{DGifOpenFileName(path.string().c_str(), &error), close_gif};
    if (!gif || DGifSlurp(gif.get()) == GIF_ERROR)
    {
        throw std::runtime_error("Can't decode GIF test input: " + path.string());
    }
    return gif;
}

void expect_gif_raster(const GifFileType &gif, const std::vector<Byte> &pixels, const int width, const int height)
{
    EXPECT_EQ(width, gif.SWidth);
    EXPECT_EQ(height, gif.SHeight);
    ASSERT_EQ(1, gif.ImageCount);
    const SavedImage &image{gif.SavedImages[0]};
    EXPECT_EQ(0, image.ImageDesc.Left);
    EXPECT_EQ(0, image.ImageDesc.Top);
    EXPECT_EQ(width, image.ImageDesc.Width);
    EXPECT_EQ(height, image.ImageDesc.Height);
    EXPECT_FALSE(image.ImageDesc.Interlace);
    EXPECT_EQ(nullptr, image.ImageDesc.ColorMap);
    EXPECT_EQ(
        pixels, (std::vector<Byte>{image.RasterBits, image.RasterBits + static_cast<std::size_t>(width) * height}));
}

} // namespace id::test
