// SPDX-License-Identifier: GPL-3.0-only
//
#include <image-tool/GIFInputFile.h>
#include <image-tool/gif_compare.h>
#include <image-tool/gif_json.h>

#include <algorithm>
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

struct RgbImage
{
    int width{};
    int height{};
    std::vector<std::uint8_t> pixels;
};

static void write_png(const std::filesystem::path &path, const RgbImage &image)
{
    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = image.width;
    png.height = image.height;
    png.format = PNG_FORMAT_RGB;
    if (png_image_write_to_file(&png, path.string().c_str(), 0, image.pixels.data(), 0, nullptr) == 0)
    {
        throw std::runtime_error{"Could not write PNG " + path.string() + ": " + png.message};
    }
}

static int rgb_delta(const std::uint8_t *pixel1, const std::uint8_t *pixel2)
{
    return std::abs(pixel1[0] - pixel2[0]) //
        + std::abs(pixel1[1] - pixel2[1]) //
        + std::abs(pixel1[2] - pixel2[2]);
}

static RgbImage make_difference_image(const RgbImage &image1, const RgbImage &image2)
{
    RgbImage result{image1.width, image1.height};
    result.pixels.resize(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 3U);
    const int num_pixels{image1.width * image1.height};
    for (int i = 0; i < num_pixels; ++i)
    {
        const std::size_t offset{static_cast<std::size_t>(i) * 3U};
        const int delta{rgb_delta(&image1.pixels[offset], &image2.pixels[offset])};
        if (delta == 0)
        {
            result.pixels[offset + 0] = 0;
            result.pixels[offset + 1] = 255;
            result.pixels[offset + 2] = 0;
        }
        else
        {
            result.pixels[offset + 0] = 255;
            result.pixels[offset + 1] = 0;
            result.pixels[offset + 2] = static_cast<std::uint8_t>(std::min(255, delta * 255 / 765));
        }
    }
    return result;
}

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

static int compare_rgb_images(
    const std::string &file1, const std::string &file2, const std::filesystem::path &diff_image)
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
    long long total_delta{};
    int max_delta{};
    int first_mismatch{-1};
    int min_x{image1.width};
    int min_y{image1.height};
    int max_x{};
    int max_y{};
    std::array<int, 766> histogram{};
    const int num_pixels{image1.width * image1.height};
    for (int i = 0; i < num_pixels; ++i)
    {
        const std::size_t offset{static_cast<std::size_t>(i) * 3U};
        const int delta{rgb_delta(&image1.pixels[offset], &image2.pixels[offset])};
        if (image1.pixels[offset + 0] != image2.pixels[offset + 0] //
            || image1.pixels[offset + 1] != image2.pixels[offset + 1]
            || image1.pixels[offset + 2] != image2.pixels[offset + 2])
        {
            if (first_mismatch < 0)
            {
                first_mismatch = i;
            }
            const int x{i % image1.width};
            const int y{i / image1.width};
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
            ++pixel_count;
            total_delta += delta;
            max_delta = std::max(max_delta, delta);
            ++histogram[delta];
        }
    }
    if (pixel_count > 0)
    {
        const float percentage{100.0f * static_cast<float>(pixel_count) / static_cast<float>(num_pixels)};
        if (!diff_image.empty())
        {
            std::filesystem::path diff_path{diff_image};
            write_png(diff_path.make_preferred(), make_difference_image(image1, image2));
            std::cout << "Wrote difference image to " << diff_image << '\n';
        }
        std::cout << fmt::format("Images differ by {:f}% ({:d}/{:d} pixels)\n"
                                 "{:s}\n"
                                 "{:s}\n",
            percentage, pixel_count, num_pixels, file1, file2);
        std::cout << fmt::format("RGB delta: avg={:.3f}, max={:d}\n",
            static_cast<double>(total_delta) / static_cast<double>(num_pixels), max_delta);
        std::cout << fmt::format("Difference bounds: ({:d},{:d}) - ({:d},{:d})\n", min_x, min_y, max_x, max_y);
        const int first_x{first_mismatch % image1.width};
        const int first_y{first_mismatch / image1.width};
        const std::size_t first_offset{static_cast<std::size_t>(first_mismatch) * 3U};
        std::cout << fmt::format("First mismatch at ({:d},{:d}): ({:d},{:d},{:d}) != ({:d},{:d},{:d})\n",
            first_x, first_y, image1.pixels[first_offset], image1.pixels[first_offset + 1],
            image1.pixels[first_offset + 2], image2.pixels[first_offset], image2.pixels[first_offset + 1],
            image2.pixels[first_offset + 2]);
        std::cout << "RGB delta histogram:\n";
        for (std::size_t i = 0; i < histogram.size(); ++i)
        {
            if (histogram[i] > 0)
            {
                std::cout << fmt::format("  [{:3d}]: {:d}\n", i, histogram[i]);
            }
        }
        return 1;
    }

    std::cout << file1 << " compares equal to " << file2 << '\n';
    return 0;
}

static const ColorMapObject &image_color_map(const SavedImage &image, const ColorMapObject &fallback)
{
    return image.ImageDesc.ColorMap != nullptr ? *image.ImageDesc.ColorMap : fallback;
}

static void set_indexed_pixel(
    RgbImage &image, const SavedImage &saved_image, const ColorMapObject &color_map, const int pixel)
{
    const int color_index{saved_image.RasterBits[pixel]};
    if (color_index < 0 || color_index >= color_map.ColorCount)
    {
        throw std::runtime_error{"GIF color index out of range."};
    }
    const GifColorType &color{color_map.Colors[color_index]};
    const std::size_t offset{static_cast<std::size_t>(pixel) * 3U};
    image.pixels[offset + 0] = color.Red;
    image.pixels[offset + 1] = color.Green;
    image.pixels[offset + 2] = color.Blue;
}

static RgbImage indexed_to_rgb(const SavedImage &image, const ColorMapObject &color_map)
{
    RgbImage result{image.ImageDesc.Width, image.ImageDesc.Height};
    result.pixels.resize(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 3U);
    const int num_pixels{result.width * result.height};
    for (int i = 0; i < num_pixels; ++i)
    {
        set_indexed_pixel(result, image, color_map, i);
    }
    return result;
}

static RgbImage indexed_to_grayscale(const SavedImage &image)
{
    RgbImage result{image.ImageDesc.Width, image.ImageDesc.Height};
    result.pixels.resize(static_cast<std::size_t>(result.width) * static_cast<std::size_t>(result.height) * 3U);
    const int num_pixels{result.width * result.height};
    for (int i = 0; i < num_pixels; ++i)
    {
        const std::uint8_t color{image.RasterBits[i]};
        const std::size_t offset{static_cast<std::size_t>(i) * 3U};
        result.pixels[offset + 0] = color;
        result.pixels[offset + 1] = color;
        result.pixels[offset + 2] = color;
    }
    return result;
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
            return compare_rgb_images(file1, file2, diff_image);
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
            long long total_delta{};
            int max_delta{};
            int first_mismatch{-1};
            int min_x{image1.ImageDesc.Width};
            int min_y{image1.ImageDesc.Height};
            int max_x{};
            int max_y{};
            std::array<int, 766> histogram{};
            const int num_pixels{image1.ImageDesc.Width * image1.ImageDesc.Height};
            const RgbImage rgb1{compare_colormap ? indexed_to_rgb(image1, image_color_map(image1, gif1.color_map())) :
                                                 indexed_to_grayscale(image1)};
            const RgbImage rgb2{compare_colormap ? indexed_to_rgb(image2, image_color_map(image2, gif2.color_map())) :
                                                 indexed_to_grayscale(image2)};
            for (int j = 0; j < num_pixels; ++j)
            {
                const std::size_t offset{static_cast<std::size_t>(j) * 3U};
                const int diff{rgb_delta(&rgb1.pixels[offset], &rgb2.pixels[offset])};
                if (diff != 0)
                {
                    ++pixel_count;
                    if (first_mismatch < 0)
                    {
                        first_mismatch = j;
                    }
                    const int x{j % image1.ImageDesc.Width};
                    const int y{j / image1.ImageDesc.Width};
                    min_x = std::min(min_x, x);
                    min_y = std::min(min_y, y);
                    max_x = std::max(max_x, x);
                    max_y = std::max(max_y, y);
                    total_delta += diff;
                    max_delta = std::max(max_delta, diff);
                    assert(diff < static_cast<int>(histogram.size()));
                    histogram[diff]++;
                }
            }

            const float percentage{100.0f * static_cast<float>(pixel_count) / static_cast<float>(num_pixels)};
            if (percentage > 3.0f)
            {
                if (!diff_image.empty())
                {
                    write_png(diff_image.make_preferred(), make_difference_image(rgb1, rgb2));
                    std::cout << "Wrote difference image to " << diff_image << '\n';
                }
                std::cout << fmt::format("Images differ by {:f}% ({:d}/{:d} pixels)\n"
                                         "{:s}\n"
                                         "{:s}\n",
                    percentage, pixel_count, num_pixels, file1, file2);
                std::cout << fmt::format("Pixel delta: avg={:.3f}, max={:d}\n",
                    static_cast<double>(total_delta) / static_cast<double>(num_pixels), max_delta);
                std::cout << fmt::format(
                    "Difference bounds: ({:d},{:d}) - ({:d},{:d})\n", min_x, min_y, max_x, max_y);
                const int first_x{first_mismatch % image1.ImageDesc.Width};
                const int first_y{first_mismatch / image1.ImageDesc.Width};
                const std::size_t first_offset{static_cast<std::size_t>(first_mismatch) * 3U};
                if (compare_colormap)
                {
                    std::cout << fmt::format(
                        "First mismatch at ({:d},{:d}): index {:d} ({:d},{:d},{:d}) != index {:d} ({:d},{:d},{:d})\n",
                        first_x, first_y, image1.RasterBits[first_mismatch], rgb1.pixels[first_offset],
                        rgb1.pixels[first_offset + 1], rgb1.pixels[first_offset + 2],
                        image2.RasterBits[first_mismatch], rgb2.pixels[first_offset], rgb2.pixels[first_offset + 1],
                        rgb2.pixels[first_offset + 2]);
                }
                else
                {
                    std::cout << fmt::format("First mismatch at ({:d},{:d}): {:d} != {:d}\n",
                        first_x, first_y, image1.RasterBits[first_mismatch], image2.RasterBits[first_mismatch]);
                }
                std::cout << "Pixel difference histogram:\n";
                for (std::size_t j = 0; j < histogram.size(); ++j)
                {
                    if (histogram[j] > 0)
                    {
                        std::cout << fmt::format("  [{:3d}]: {:d}\n", static_cast<int>(j), histogram[j]);
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
