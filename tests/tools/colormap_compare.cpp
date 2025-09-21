// SPDX-License-Identifier: GPL-3.0-only
//
#include <image-tool/GIFInputFile.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace id
{

static int usage(const std::string_view program)
{
    std::cout << "Usage: " << program << ": map-file image-file\n";
    return 1;
}

struct Color
{
    int red;
    int green;
    int blue;
};

} // namespace id

int main(const int argc, char *argv[])
{
    using namespace id;
    if (argc != 3)
    {
        return usage(argv[0]);
    }
    const std::string map_file{argv[1]};
    const std::string image_file{argv[2]};
    try
    {
        if (!std::filesystem::exists(map_file))
        {
            std::cout << map_file << " does not exist.\n";
            return 1;
        }
        if (!std::filesystem::exists(image_file))
        {
            std::cout << image_file << " does not exist.\n";
            return 1;
        }

        GIFInputFile image{image_file};
        image.slurp();

        std::ifstream map{map_file};
        std::vector<Color> colormap;
        while (map && colormap.size() < 256)
        {
            int red{-1};
            map >> red;
            if (!map)
            {
                break;
            }
            int green{-1};
            map >> green;
            if (!map)
            {
                std::cout << "Error reading green value from " << map_file << '\n';
                return 1;
            }
            int blue{-1};
            map >> blue;
            if (!map)
            {
                std::cout << "Error reading blue value from " << map_file << '\n';
                return 1;
            }
            colormap.push_back(Color{red, green, blue});
            std::string line;
            std::getline(map, line);
        }
        if (image.color_map().ColorCount != static_cast<int>(colormap.size()))
        {
            std::cout << "Color count doesn't match\n"
                      << map_file << ": " << colormap.size() << '\n'
                      << image_file << ": " << image.color_map().ColorCount << '\n';
            return 1;
        }

        for (int i = 0; i < image.color_map().ColorCount; ++i)
        {
            if (image.color_map().Colors[i].Red != colormap[i].red ||
                image.color_map().Colors[i].Green != colormap[i].green ||
                image.color_map().Colors[i].Blue != colormap[i].blue)
            {
                std::cout << "Color mismatch at index " << i << '\n'
                          << map_file << ": " << colormap[i].red << ' '
                          << colormap[i].green << ' ' << colormap[i].blue << '\n'
                          << image_file << ": "
                          << static_cast<int>(image.color_map().Colors[i].Red) << ' '
                          << static_cast<int>(image.color_map().Colors[i].Green) << ' '
                          << static_cast<int>(image.color_map().Colors[i].Blue) << '\n';
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

    std::cout << map_file << " compares equal to " << image_file << '\n';
    return 0;
}
