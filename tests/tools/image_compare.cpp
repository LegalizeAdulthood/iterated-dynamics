// SPDX-License-Identifier: GPL-3.0-only
//
#include <image-tool/GIFInputFile.h>
#include <image-tool/gif_compare.h>
#include <image-tool/gif_json.h>

#include <array>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fmt/format.h>
#include <iostream>
#include <png.h>
#include <string>
#include <string_view>
#include <tga.h>
#include <vector>

namespace id
{

static int usage(const std::string_view program)
{
    std::cout << "Usage: " << program << ": [--ignore-colormap] [--diff-image file] file1 file2\n";
    return 1;
}

static ColorMapObject *make_grayscale_color_map()
{
    ColorMapObject *color_map = GifMakeMapObject(256, nullptr);
    if (color_map == nullptr)
    {
        throw std::runtime_error{"Could not allocate GIF color map."};
    }
    for (int i = 0; i < 256; ++i)
    {
        color_map->Colors[i].Red = i;
        color_map->Colors[i].Green = i;
        color_map->Colors[i].Blue = i;
    }
    return color_map;
}

struct RgbImage
{
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels;
};

static std::string lower_extension(const std::filesystem::path &path)
{
    std::string ext{path.extension().string()};
    for (char &ch : ext)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return ext;
}

static bool is_true_color_path(const std::filesystem::path &path)
{
    const std::string ext{lower_extension(path)};
    return ext == ".png" || ext == ".tga";
}

static RgbImage load_png(const std::filesystem::path &path)
{
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    if (png_image_begin_read_from_file(&image, path.string().c_str()) == 0)
    {
        throw std::runtime_error{"Could not read PNG " + path.string() + ": " + image.message};
    }
    image.format = PNG_FORMAT_RGB;
    RgbImage result{static_cast<int>(image.width), static_cast<int>(image.height)};
    result.pixels.resize(PNG_IMAGE_SIZE(image));
    if (png_image_finish_read(&image, nullptr, result.pixels.data(), 0, nullptr) == 0)
    {
        const std::string message{image.message};
        png_image_free(&image);
        throw std::runtime_error{"Could not read PNG pixels " + path.string() + ": " + message};
    }
    png_image_free(&image);
    return result;
}

static std::uint8_t scale_5_to_8(const unsigned int value)
{
    return static_cast<std::uint8_t>((value << 3) | (value >> 2));
}

static void set_tga_pixel(RgbImage &image, const TGAFile &tga, const int file_y, const int file_x, const unsigned char *src)
{
    const bool right_to_left{(tga.imageDesc & 0x10) != 0};
    const bool top_to_bottom{(tga.imageDesc & 0x20) != 0};
    const int x{right_to_left ? image.width - 1 - file_x : file_x};
    const int y{top_to_bottom ? file_y : image.height - 1 - file_y};
    const std::size_t offset{(static_cast<std::size_t>(y) * image.width + x) * 3U};
    if (tga.pixelDepth == 16)
    {
        const unsigned int value{static_cast<unsigned int>(src[0]) | (static_cast<unsigned int>(src[1]) << 8)};
        image.pixels[offset + 0] = scale_5_to_8((value >> 10) & 0x1f);
        image.pixels[offset + 1] = scale_5_to_8((value >> 5) & 0x1f);
        image.pixels[offset + 2] = scale_5_to_8(value & 0x1f);
        return;
    }
    image.pixels[offset + 0] = src[2];
    image.pixels[offset + 1] = src[1];
    image.pixels[offset + 2] = src[0];
}

static RgbImage load_tga(const std::filesystem::path &path)
{
    FILE *file{std::fopen(path.string().c_str(), "rb")};
    if (file == nullptr)
    {
        throw std::runtime_error{"Could not open TGA " + path.string()};
    }
    TGAFile tga{};
    const int read_result{ReadTGAFile(file, &tga)};
    if (read_result < 0)
    {
        std::fclose(file);
        throw std::runtime_error{"Could not read TGA " + path.string() + ": " + std::to_string(read_result)};
    }
    if (tga.imageType != 2 && tga.imageType != 10)
    {
        FreeTGAFile(&tga);
        std::fclose(file);
        throw std::runtime_error{"Unsupported TGA image type in " + path.string() + ": " + std::to_string(tga.imageType)};
    }
    if (tga.pixelDepth != 16 && tga.pixelDepth != 24 && tga.pixelDepth != 32)
    {
        FreeTGAFile(&tga);
        std::fclose(file);
        throw std::runtime_error{"Unsupported TGA pixel depth in " + path.string() + ": " + std::to_string(tga.pixelDepth)};
    }

    const int bytes_per_pixel{(tga.pixelDepth + 7) / 8};
    const long image_offset{18L + tga.idLength + ((tga.mapWidth + 7) >> 3) * static_cast<long>(tga.mapLength)};
    if (std::fseek(file, image_offset, SEEK_SET) != 0)
    {
        FreeTGAFile(&tga);
        std::fclose(file);
        throw std::runtime_error{"Could not seek to TGA pixels in " + path.string()};
    }

    RgbImage result{tga.imageWidth, tga.imageHeight};
    result.pixels.resize(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 3U);
    std::vector<unsigned char> row(static_cast<std::size_t>(result.width) * bytes_per_pixel);
    for (int y = 0; y < result.height; ++y)
    {
        const int row_bytes{result.width * bytes_per_pixel};
        int row_result{};
        if (tga.imageType == 10)
        {
            row_result = ReadRLERow(file, row.data(), row_bytes, bytes_per_pixel);
        }
        else
        {
            row_result = static_cast<int>(std::fread(row.data(), 1, row_bytes, file)) == row_bytes ? 0 : -1;
        }
        if (row_result < 0)
        {
            FreeTGAFile(&tga);
            std::fclose(file);
            throw std::runtime_error{"Could not read TGA pixel row from " + path.string()};
        }
        for (int x = 0; x < result.width; ++x)
        {
            set_tga_pixel(result, tga, y, x, &row[static_cast<std::size_t>(x) * bytes_per_pixel]);
        }
    }

    FreeTGAFile(&tga);
    std::fclose(file);
    return result;
}

static RgbImage load_rgb_image(const std::filesystem::path &path)
{
    const std::string ext{lower_extension(path)};
    if (ext == ".png")
    {
        return load_png(path);
    }
    if (ext == ".tga")
    {
        return load_tga(path);
    }
    throw std::runtime_error{"Unsupported true-color image type: " + path.string()};
}

static int compare_rgb_images(const std::string &file1, const std::string &file2)
{
    const RgbImage image1{load_rgb_image(file1)};
    const RgbImage image2{load_rgb_image(file2)};
    if (image1.width != image2.width || image1.height != image2.height)
    {
        std::cout << "Image dimensions don't match: "                                    //
                  << file1 << ": " << image1.width << "x" << image1.height << " != " //
                  << file2 << ": " << image2.width << "x" << image2.height << '\n';
        return 1;
    }

    int pixel_count{};
    const int num_pixels{image1.width * image1.height};
    for (int i = 0; i < num_pixels; ++i)
    {
        const std::size_t offset{static_cast<std::size_t>(i) * 3U};
        if (image1.pixels[offset + 0] != image2.pixels[offset + 0] //
            || image1.pixels[offset + 1] != image2.pixels[offset + 1]
            || image1.pixels[offset + 2] != image2.pixels[offset + 2])
        {
            ++pixel_count;
        }
    }
    if (pixel_count > 0)
    {
        const float percentage{100.0f * static_cast<float>(pixel_count) / static_cast<float>(num_pixels)};
        std::cout << fmt::format("Images differ by {:f}% ({:d}/{:d} pixels)\n"
                                 "{:s}\n"
                                 "{:s}\n",
            percentage, pixel_count, num_pixels, file1, file2);
        return 1;
    }

    std::cout << file1 << " compares equal to " << file2 << '\n';
    return 0;
}

static void write_difference_gif(const std::filesystem::path &path, const SavedImage &image1, const SavedImage &image2)
{
    const int width{image1.ImageDesc.Width};
    const int height{image1.ImageDesc.Height};
    std::vector<GifByteType> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (std::size_t i = 0; i < pixels.size(); ++i)
    {
        pixels[i] = static_cast<GifByteType>(std::abs(image1.RasterBits[i] - image2.RasterBits[i]));
    }

    int gif_error{};
    GifFileType *gif = EGifOpenFileName(path.string().c_str(), false, &gif_error);
    if (gif == nullptr)
    {
        throw std::runtime_error{"Could not open " + path.string() + " for writing: " + std::to_string(gif_error)};
    }

    ColorMapObject *color_map = make_grayscale_color_map();
    int result = EGifPutScreenDesc(gif, width, height, 8, 0, color_map);
    GifFreeMapObject(color_map);
    if (result == GIF_ERROR)
    {
        EGifCloseFile(gif, &gif_error);
        throw std::runtime_error{"Could not write GIF screen description to " + path.string()};
    }

    if (EGifPutImageDesc(gif, 0, 0, width, height, false, nullptr) == GIF_ERROR)
    {
        EGifCloseFile(gif, &gif_error);
        throw std::runtime_error{"Could not write GIF image description to " + path.string()};
    }

    for (int row = 0; row < height; ++row)
    {
        if (EGifPutLine(gif, &pixels[static_cast<std::size_t>(row) * static_cast<std::size_t>(width)], width) == GIF_ERROR)
        {
            EGifCloseFile(gif, &gif_error);
            throw std::runtime_error{"Could not write GIF raster data to " + path.string()};
        }
    }

    if (EGifCloseFile(gif, &gif_error) == GIF_ERROR)
    {
        throw std::runtime_error{"Could not close " + path.string() + " after writing: " + std::to_string(gif_error)};
    }
}

} // namespace id

int main(const int argc, char *argv[])
{
    using namespace id;
    if (argc < 3)
    {
        return usage(argv[0]);
    }
    bool compare_colormap{true};
    int start_arg{1};
    std::filesystem::path diff_image;
    while (start_arg < argc && std::string_view{argv[start_arg]}.substr(0, 2) == "--")
    {
        if (std::string{argv[start_arg]} == "--ignore-colormap")
        {
            compare_colormap = false;
            ++start_arg;
        }
        else if (std::string{argv[start_arg]} == "--diff-image")
        {
            ++start_arg;
            if (start_arg >= argc)
            {
                return usage(argv[0]);
            }
            diff_image = argv[start_arg++];
        }
        else
        {
            return usage(argv[0]);
        }
    }
    if (start_arg + 2 != argc)
    {
        return usage(argv[0]);
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

        if (is_true_color_path(file1) || is_true_color_path(file2))
        {
            return compare_rgb_images(file1, file2);
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
                if (!diff_image.empty())
                {
                    write_difference_gif(diff_image.make_preferred(), image1, image2);
                    std::cout << "Wrote difference image to " << diff_image << '\n';
                }
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
