// SPDX-License-Identifier: GPL-3.0-only
//
#include <image-tool/GIFInputFile.h>
#include <image-tool/gif_compare.h>
#include <image-tool/gif_format.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 2)
    {
        std::cout << "Usage: " << argv[0] << ": file\n";
        return 1;
    }
    const std::string file{argv[1]};
    if (!std::filesystem::exists(file))
    {
        std::cout << file << " does not exist.\n";
        return 1;
    }

    try
    {
        id::GIFInputFile gif{file};
        gif.slurp();

        std::cout << file << ":\n";
        std::cout << "  Screen width: " << gif.screen_width() << '\n';
        std::cout << "  Screen height: " << gif.screen_height() << '\n';
        std::cout << "  Color resolution: " << gif.color_resolution() << '\n';
        std::cout << "  Color map: " << gif.color_map() << '\n';
        std::cout << "  Number of images: " << gif.num_images() << '\n';
        for (int i = 0; i < gif.num_images(); ++i)
        {
            const SavedImage &image{gif.get_image(i)};
            std::cout << "  Image " << i << ": " << image.ImageDesc << '\n';
            for (int y = 0; y < image.ImageDesc.Height; ++y)
            {
                std::cout << fmt::format("  {:4}: ", y);
                for (int x = 0; x < image.ImageDesc.Width; ++x)
                {
                    const GifByteType pixel{image.RasterBits[y * image.ImageDesc.Width + x]};
                    if (x > 0)
                    {
                        if (x % 16 == 0)
                        {
                            std::cout << "\n        ";
                        }
                        else
                        {
                            std::cout << ' ';
                        }
                    }
                    std::cout << fmt::format("{:3}", pixel);
                }
                std::cout << '\n';
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

    return 0;
}
