// SPDX-License-Identifier: GPL-3.0-only
//
#include <image-tool/GIFInputFile.h>
#include <image-tool/gif_compare.h>
#include <image-tool/gif_json.h>

#include <array>
#include <cassert>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <string>
#include <string_view>

namespace id
{

static int usage(const std::string_view program)
{
    std::cout << "Usage: " << program << ": [--ignore-colormap] file1 file2\n";
    return 1;
}

} // namespace id

int main(const int argc, char *argv[])
{
    using namespace id;
    if (argc < 3 || argc > 4)
    {
        return usage(argv[0]);
    }
    bool compare_colormap{true};
    int start_arg{1};
    if (argc == 4)
    {
        if (std::string{argv[1]} == "--ignore-colormap")
        {
            compare_colormap = false;
            ++start_arg;
        }
        else
        {
            return usage(argv[0]);
        }
    }
    const std::string file1{argv[start_arg]};
    const std::string file2{argv[start_arg + 1]};
    try
    {
        if (!std::filesystem::exists(file1))
        {
            std::cout << file1 << " does not exist.\n";
            return 1;
        }
        if (!std::filesystem::exists(file2))
        {
            std::cout << file2 << " does not exist.\n";
            return 1;
        }

        GIFInputFile gif1{file1};
        gif1.slurp();
        GIFInputFile gif2{file2};
        gif2.slurp();

        if (gif1.screen_width() != gif2.screen_width() || gif1.screen_height() != gif2.screen_height())
        {
            std::cout << "Image dimensions don't match: "                                        //
                      << file1 << ": " << gif1.screen_width() << "x" << gif1.screen_height() //
                      << " != "                                                                  //
                      << file2 << gif1.screen_width() << "x" << gif1.screen_height()         //
                      << '\n';
            return 1;
        }

        if (compare_colormap && gif1.color_map() != gif2.color_map())
        {
            const ColorMapObject &colormap1{gif1.color_map()};
            const ColorMapObject &colormap2{gif2.color_map()};
            if (colormap1.ColorCount != colormap2.ColorCount        //
                || colormap1.BitsPerPixel != colormap2.BitsPerPixel //
                || colormap1.SortFlag != colormap2.SortFlag)
            {
                std::cout << "Color table metadata doesn't match\n"
                          << file1 << ": count=" << colormap1.ColorCount
                          << ", bits=" << colormap1.BitsPerPixel
                          << ", sort=" << (colormap1.SortFlag ? "yes" : "no") << '\n'
                          << file2 << ": count=" << colormap2.ColorCount
                          << ", bits=" << colormap2.BitsPerPixel
                          << ", sort=" << (colormap2.SortFlag ? "yes" : "no") << '\n';
                return 1;
            }
            std::cout << file1 << " != " << file2 << '\n';
            for (int i = 0; i < colormap1.ColorCount; ++i)
            {
                if (colormap1.Colors[i] != colormap2.Colors[i])
                {
                    std::cout << "[" << i << "] " << colormap1.Colors[i] << " != " << colormap2.Colors[i] << '\n';
                }
            }
            return 1;
        }

        if (gif1.num_images() != gif2.num_images())
        {
            std::cout << "Image count doesn't match\n"
                      << file1 << ": " << gif1.num_images() << '\n'
                      << file2 << ": " << gif2.num_images() << '\n';
            return 1;
        }

        for (int i = 0; i < gif1.num_images(); ++i)
        {
            const SavedImage &image1{gif1.get_image(i)};
            const SavedImage &image2{gif2.get_image(i)};
            if (image1.ImageDesc != image2.ImageDesc)
            {
                std::cout << "Image " << i << " description doesn't match\n"
                          << file1 << ": " << image1.ImageDesc << '\n'
                          << file2 << ": " << image2.ImageDesc << '\n';
                return 1;
            }
            int pixel_count{};
            std::array<int, 256> histogram{};
            const int num_pixels{image1.ImageDesc.Width * image1.ImageDesc.Height};
            for (int j = 0; j < num_pixels; ++j)
            {
                if (image1.RasterBits[j] != image2.RasterBits[j])
                {
                    ++pixel_count;
                    const int diff = std::abs(image1.RasterBits[j] - image2.RasterBits[j]);
                    assert(diff < 256);
                    histogram[diff]++;
                }
            }
            const float percentage{100.0f * static_cast<float>(pixel_count) / static_cast<float>(num_pixels)};
            if (percentage > 3.0f)
            {
                std::cout << fmt::format("Images differ by {:f}% ({:d}/{:d} pixels)\n"
                                         "{:s}\n"
                                         "{:s}\n",
                    percentage, pixel_count, num_pixels, file1, file2);
                std::cout << "Pixel difference histogram:\n";
                for (int j = 0; j < 256; ++j)
                {
                    if (histogram[j] > 0)
                    {
                        std::cout << fmt::format("  [{:3d}]: {:d}\n", j, histogram[j]);
                    }
                }
                return 1;
            }
        }
    }
    catch (const std::exception &bang)
    {
        std::cout << "Unexpected exception: " << bang.what() << '\n';
        return 100;
    }
    catch (...)
    {
        std::cout << "Unexpected exception\n";
        return 200;
    }

    std::cout << file1 << " compares equal to " << file2 << '\n';
    return 0;
}
