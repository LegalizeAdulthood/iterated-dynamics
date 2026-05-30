// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/make_mig.h"

#include "io/check_write_file.h"
#include "io/library.h"

#include <fmt/format.h>
#include <gif_lib.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>

namespace id::io
{

class InputGif
{
public:
    explicit InputGif(const std::filesystem::path &path) :
        m_path(path),
        m_gif(DGifOpenFileName(m_path.string().c_str(), &m_error))
    {
    }

    ~InputGif()
    {
        close();
    }

    InputGif(const InputGif &rhs) = delete;
    InputGif(InputGif &&rhs) noexcept :
        m_path(std::move(rhs.m_path)),
        m_gif(std::exchange(rhs.m_gif, nullptr)),
        m_error(rhs.m_error),
        m_slurp_result(rhs.m_slurp_result)
    {
    }
    InputGif &operator=(const InputGif &rhs) = delete;
    InputGif &operator=(InputGif &&rhs) = delete;

    // clang-format off
    const std::filesystem::path &path() const  { return m_path; }
    GifFileType *get() const                   { return m_gif; }
    int error() const                          { return m_error; }
    int slurp_result() const                   { return m_slurp_result; }
    // clang-format on

    int slurp()
    {
        if (m_gif == nullptr)
        {
            m_slurp_result = GIF_ERROR;
            return m_slurp_result;
        }
        m_slurp_result = DGifSlurp(m_gif);
        return m_slurp_result;
    }

private:
    void close()
    {
        if (m_gif != nullptr)
        {
            int close_error{};
            DGifCloseFile(m_gif, &close_error);
            m_gif = nullptr;
        }
    }

    std::filesystem::path m_path;
    GifFileType *m_gif{};
    int m_error{};
    int m_slurp_result{GIF_ERROR};
};

static InputGif read_input_gif(const std::filesystem::path &path)
{
    InputGif gif{path};
    gif.slurp();
    return gif;
}

class OutputGif
{
public:
    explicit OutputGif(const std::filesystem::path &path) :
        m_path(path),
        m_gif(EGifOpenFileName(m_path.string().c_str(), false, &m_error))
    {
    }

    ~OutputGif()
    {
        close();
    }

    OutputGif(const OutputGif &rhs) = delete;
    OutputGif(OutputGif &&rhs) noexcept :
        m_path(std::move(rhs.m_path)),
        m_gif(std::exchange(rhs.m_gif, nullptr)),
        m_error(rhs.m_error),
        m_spew_result(rhs.m_spew_result)
    {
    }
    OutputGif &operator=(const OutputGif &rhs) = delete;
    OutputGif &operator=(OutputGif &&rhs) = delete;

    // clang-format off
    const std::filesystem::path &path() const  { return m_path; }
    GifFileType *get() const                   { return m_gif; }
    int error() const                          { return m_error; }
    int spew_result() const                    { return m_spew_result; }
    // clang-format on

    int spew()
    {
        if (m_gif == nullptr)
        {
            m_spew_result = GIF_ERROR;
            return m_spew_result;
        }
        m_spew_result = EGifSpew(m_gif);
        m_gif = nullptr;
        return m_spew_result;
    }

private:
    void close()
    {
        if (m_gif != nullptr)
        {
            int close_error{};
            EGifCloseFile(m_gif, &close_error);
            m_gif = nullptr;
        }
    }

    std::filesystem::path m_path;
    GifFileType *m_gif{};
    int m_error{};
    int m_spew_result{GIF_ERROR};
};

static char par_key(const unsigned int x)
{
    return x < 10 ? '0' + x : 'a' - 10 + x;
}

struct MigMetadata
{
    int width{};
    int height{};
    int color_count{};
    int color_resolution{};
    int background_color{};
    int aspect_byte{};
};

static MigMetadata get_metadata(const GifFileType *gif)
{
    return MigMetadata{
        gif->SWidth,
        gif->SHeight,
        gif->SColorMap == nullptr ? 0 : gif->SColorMap->ColorCount,
        gif->SColorResolution,
        gif->SBackGroundColor,
        gif->AspectByte};
}

static std::string input_gif_name(const unsigned int x_step, const unsigned int y_step)
{
    return fmt::format("frmig_{:c}{:c}.gif", par_key(x_step), par_key(y_step));
}

static void validate_canvas_size(const MigMetadata &metadata, const unsigned int x_mult, const unsigned int y_mult)
{
    constexpr std::uint64_t MAX_GIF_DIMENSION{std::numeric_limits<std::uint16_t>::max()};
    const std::uint64_t width = static_cast<std::uint64_t>(metadata.width) * x_mult;
    const std::uint64_t height = static_cast<std::uint64_t>(metadata.height) * y_mult;
    if (width > MAX_GIF_DIMENSION || height > MAX_GIF_DIMENSION)
    {
        fmt::print("MIG output dimensions {:d} x {:d} exceed GIF limits!\n", width, height);
        std::exit(1);
    }
}

static void set_output_screen(
    GifFileType *output, const GifFileType *first_input, const unsigned int x_mult, const unsigned int y_mult)
{
    const MigMetadata metadata{get_metadata(first_input)};
    validate_canvas_size(metadata, x_mult, y_mult);
    output->SWidth = metadata.width * x_mult;
    output->SHeight = metadata.height * y_mult;
    output->SColorResolution = metadata.color_resolution;
    output->SBackGroundColor = metadata.background_color;
    output->AspectByte = metadata.aspect_byte;
    if (first_input->SColorMap != nullptr)
    {
        output->SColorMap = GifMakeMapObject(
            first_input->SColorMap->ColorCount, first_input->SColorMap->Colors);
        if (output->SColorMap == nullptr)
        {
            std::printf("Cannot allocate output color map!\n");
            std::exit(1);
        }
    }
}

static OutputGif create_output_gif(
    const std::filesystem::path &path,
    const GifFileType *first_input,
    const unsigned int x_mult,
    const unsigned int y_mult)
{
    OutputGif output{path};
    if (output.get() == nullptr)
    {
        std::printf("Cannot create output file %s!\n", path.filename().string().c_str());
        std::exit(1);
    }
    set_output_screen(output.get(), first_input, x_mult, y_mult);
    return output;
}

static std::size_t raster_size(const SavedImage &image)
{
    return static_cast<std::size_t>(image.ImageDesc.Width) * image.ImageDesc.Height;
}

static bool same_color_map(const ColorMapObject *lhs, const ColorMapObject *rhs)
{
    if (lhs == nullptr || rhs == nullptr)
    {
        return lhs == rhs;
    }
    return lhs->ColorCount == rhs->ColorCount &&
        std::memcmp(lhs->Colors, rhs->Colors, lhs->ColorCount * sizeof(GifColorType)) == 0;
}

static bool same_extension_blocks(
    const int lhs_count,
    const ExtensionBlock *lhs_blocks,
    const int rhs_count,
    const ExtensionBlock *rhs_blocks)
{
    if (lhs_count != rhs_count)
    {
        return false;
    }
    for (int i = 0; i < lhs_count; i++)
    {
        if (lhs_blocks[i].Function != rhs_blocks[i].Function ||
            lhs_blocks[i].ByteCount != rhs_blocks[i].ByteCount ||
            std::memcmp(lhs_blocks[i].Bytes, rhs_blocks[i].Bytes, lhs_blocks[i].ByteCount) != 0)
        {
            return false;
        }
    }
    return true;
}

static bool same_image(const SavedImage &lhs, const SavedImage &rhs)
{
    return lhs.ImageDesc.Left == rhs.ImageDesc.Left && lhs.ImageDesc.Top == rhs.ImageDesc.Top &&
        lhs.ImageDesc.Width == rhs.ImageDesc.Width && lhs.ImageDesc.Height == rhs.ImageDesc.Height &&
        lhs.ImageDesc.Interlace == rhs.ImageDesc.Interlace &&
        same_color_map(lhs.ImageDesc.ColorMap, rhs.ImageDesc.ColorMap) &&
        same_extension_blocks(
            lhs.ExtensionBlockCount, lhs.ExtensionBlocks, rhs.ExtensionBlockCount, rhs.ExtensionBlocks) &&
        std::memcmp(lhs.RasterBits, rhs.RasterBits, raster_size(lhs)) == 0;
}

static bool same_image_at_offset(const SavedImage &lhs, const SavedImage &rhs, const int left, const int top)
{
    return lhs.ImageDesc.Left == rhs.ImageDesc.Left + left && lhs.ImageDesc.Top == rhs.ImageDesc.Top + top &&
        lhs.ImageDesc.Width == rhs.ImageDesc.Width && lhs.ImageDesc.Height == rhs.ImageDesc.Height &&
        lhs.ImageDesc.Interlace == rhs.ImageDesc.Interlace &&
        same_color_map(lhs.ImageDesc.ColorMap, rhs.ImageDesc.ColorMap) &&
        same_extension_blocks(
            lhs.ExtensionBlockCount, lhs.ExtensionBlocks, rhs.ExtensionBlockCount, rhs.ExtensionBlocks) &&
        std::memcmp(lhs.RasterBits, rhs.RasterBits, raster_size(lhs)) == 0;
}

static void clear_extensions(SavedImage *image)
{
    if (image->ExtensionBlockCount > 0)
    {
        GifFreeExtensions(&image->ExtensionBlockCount, &image->ExtensionBlocks);
    }
}

static void copy_extension_blocks(GifFileType *output, const GifFileType *input)
{
    for (int i = 0; i < input->ExtensionBlockCount; i++)
    {
        const ExtensionBlock &block{input->ExtensionBlocks[i]};
        if (GifAddExtensionBlock(
                &output->ExtensionBlockCount,
                &output->ExtensionBlocks,
                block.Function,
                block.ByteCount,
                block.Bytes) == GIF_ERROR)
        {
            std::printf("Cannot copy GIF extension block!\n");
            std::exit(1);
        }
    }
}

static SavedImage *copy_saved_image(GifFileType *output, const SavedImage &image)
{
    SavedImage *saved_image{GifMakeSavedImage(output, &image)};
    if (saved_image == nullptr)
    {
        std::printf("Cannot copy image into output GIF!\n");
        std::exit(1);
    }
    return saved_image;
}

static void copy_saved_image_at_offset(
    GifFileType *output,
    const SavedImage &image,
    const int left,
    const int top,
    const bool keep_extensions)
{
    SavedImage *saved_image{copy_saved_image(output, image)};
    saved_image->ImageDesc.Left += left;
    saved_image->ImageDesc.Top += top;
    if (!keep_extensions)
    {
        clear_extensions(saved_image);
    }
}

static void verify_single_image_output(
    const std::filesystem::path &path,
    const GifFileType *first_input,
    const unsigned int x_mult,
    const unsigned int y_mult)
{
    InputGif output{read_input_gif(path)};
    if (output.get() == nullptr || output.slurp_result() != GIF_OK)
    {
        std::printf("Cannot read output file %s!\n", path.filename().string().c_str());
        std::exit(1);
    }
    const MigMetadata input_metadata{get_metadata(first_input)};
    const MigMetadata output_metadata{get_metadata(output.get())};
    if (output_metadata.width != input_metadata.width * static_cast<int>(x_mult) ||
        output_metadata.height != input_metadata.height * static_cast<int>(y_mult) ||
        output_metadata.color_resolution != input_metadata.color_resolution ||
        output_metadata.background_color != input_metadata.background_color ||
        output_metadata.aspect_byte != input_metadata.aspect_byte ||
        !same_color_map(output.get()->SColorMap, first_input->SColorMap) || output.get()->ImageCount != 1 ||
        !same_image(output.get()->SavedImages[0], first_input->SavedImages[0]))
    {
        std::printf("Output file %s failed GIF verification!\n", path.filename().string().c_str());
        std::exit(1);
    }
}

[[maybe_unused]] static void write_single_image_gif(
    const std::filesystem::path &path,
    const GifFileType *first_input,
    const unsigned int x_mult,
    const unsigned int y_mult)
{
    if (first_input->ImageCount < 1)
    {
        std::printf("Input file contains no images!\n");
        std::exit(1);
    }
    OutputGif output{create_output_gif(path, first_input, x_mult, y_mult)};
    copy_saved_image(output.get(), first_input->SavedImages[0]);
    if (output.spew() != GIF_OK)
    {
        std::printf("Cannot write output file %s!\n", path.filename().string().c_str());
        std::exit(1);
    }
    verify_single_image_output(path, first_input, x_mult, y_mult);
}

static InputGif read_tile_gif(const unsigned int x_step, const unsigned int y_step)
{
    const std::string gif_in{input_gif_name(x_step, y_step)};
    const std::filesystem::path path{find_file(ReadFile::IMAGE, gif_in)};
    if (path.empty())
    {
        fmt::print("Can't locate file {:s}!\n", gif_in);
        std::exit(1);
    }
    InputGif gif{read_input_gif(path)};
    if (gif.get() == nullptr || gif.slurp_result() != GIF_OK || gif.get()->ImageCount < 1)
    {
        std::printf("Can't read file %s!\n", gif_in.c_str());
        std::exit(1);
    }
    return gif;
}

static int total_tile_image_count(const unsigned int x_mult, const unsigned int y_mult)
{
    int image_count{};
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            InputGif input{read_tile_gif(x_step, y_step)};
            image_count += input.get()->ImageCount;
        }
    }
    return image_count;
}

static void verify_tiled_images_output(
    const std::filesystem::path &path,
    const GifFileType *first_input,
    const unsigned int x_mult,
    const unsigned int y_mult)
{
    InputGif output{read_input_gif(path)};
    if (output.get() == nullptr || output.slurp_result() != GIF_OK)
    {
        std::printf("Cannot read output file %s!\n", path.filename().string().c_str());
        std::exit(1);
    }
    const MigMetadata input_metadata{get_metadata(first_input)};
    const MigMetadata output_metadata{get_metadata(output.get())};
    if (output_metadata.width != input_metadata.width * static_cast<int>(x_mult) ||
        output_metadata.height != input_metadata.height * static_cast<int>(y_mult) ||
        !same_color_map(output.get()->SColorMap, first_input->SColorMap) ||
        output.get()->ImageCount != total_tile_image_count(x_mult, y_mult))
    {
        std::printf("Output file %s failed GIF layout verification!\n", path.filename().string().c_str());
        std::exit(1);
    }
    int image_number{};
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            InputGif input{read_tile_gif(x_step, y_step)};
            const bool final_tile{x_step == x_mult - 1 && y_step == y_mult - 1};
            const int left{static_cast<int>(x_step) * input_metadata.width};
            const int top{static_cast<int>(y_step) * input_metadata.height};
            for (int i = 0; i < input.get()->ImageCount; i++)
            {
                SavedImage expected{input.get()->SavedImages[i]};
                if (!final_tile)
                {
                    expected.ExtensionBlockCount = 0;
                    expected.ExtensionBlocks = nullptr;
                }
                if (!same_image_at_offset(output.get()->SavedImages[image_number++], expected, left, top))
                {
                    std::printf("Output file %s failed GIF offset verification!\n", path.filename().string().c_str());
                    std::exit(1);
                }
            }
        }
    }
    InputGif final_input{read_tile_gif(x_mult - 1, y_mult - 1)};
    if (!same_extension_blocks(
            output.get()->ExtensionBlockCount,
            output.get()->ExtensionBlocks,
            final_input.get()->ExtensionBlockCount,
            final_input.get()->ExtensionBlocks))
    {
        std::printf("Output file %s failed GIF extension verification!\n", path.filename().string().c_str());
        std::exit(1);
    }
}

static void write_tiled_images_gif(
    const std::filesystem::path &path, const unsigned int x_mult, const unsigned int y_mult)
{
    InputGif first_input{read_tile_gif(0, 0)};
    const MigMetadata input_metadata{get_metadata(first_input.get())};
    OutputGif output{create_output_gif(path, first_input.get(), x_mult, y_mult)};
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            InputGif input{read_tile_gif(x_step, y_step)};
            const bool final_tile{x_step == x_mult - 1 && y_step == y_mult - 1};
            const int left{static_cast<int>(x_step) * input_metadata.width};
            const int top{static_cast<int>(y_step) * input_metadata.height};
            for (int i = 0; i < input.get()->ImageCount; i++)
            {
                copy_saved_image_at_offset(output.get(), input.get()->SavedImages[i], left, top, final_tile);
            }
            if (final_tile)
            {
                copy_extension_blocks(output.get(), input.get());
            }
        }
    }
    if (output.spew() != GIF_OK)
    {
        std::printf("Cannot write output file %s!\n", path.filename().string().c_str());
        std::exit(1);
    }
    verify_tiled_images_output(path, first_input.get(), x_mult, y_mult);
}

static MigMetadata validate_input_gifs(const unsigned int x_mult, const unsigned int y_mult)
{
    MigMetadata first{};
    bool have_first{false};
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            const std::string gif_in{input_gif_name(x_step, y_step)};
            const std::filesystem::path path{find_file(ReadFile::IMAGE, gif_in)};
            if (path.empty())
            {
                fmt::print("Can't locate file {:s}!\n", gif_in);
                std::exit(1);
            }
            InputGif gif{read_input_gif(path)};
            if (gif.get() == nullptr)
            {
                std::printf("Can't open file %s!\n", gif_in.c_str());
                std::exit(1);
            }
            if (gif.slurp_result() != GIF_OK)
            {
                std::printf("\007 Process failed = early EOF on input file %s\n", gif_in.c_str());
                std::exit(1);
            }
            const MigMetadata metadata{get_metadata(gif.get())};
            if (!have_first)
            {
                first = metadata;
                validate_canvas_size(first, x_mult, y_mult);
                have_first = true;
            }
            if (metadata.width != first.width || metadata.height != first.height ||
                metadata.color_count != first.color_count)
            {
                std::printf("File %s doesn't have the same resolution as its predecessors!\n", gif_in.c_str());
                std::exit(1);
            }
        }
    }
    return first;
}

static void delete_tile_gifs(const unsigned int x_mult, const unsigned int y_mult)
{
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            std::filesystem::remove(get_save_path(WriteFile::IMAGE, input_gif_name(x_step, y_step)));
        }
    }
}

/* make_mig() takes a collection of individual GIF images (all
   presumably the same resolution and all presumably generated
   by this program and its "divide and conquer" algorithm) and builds
   a single multiple-image GIF out of them.  This routine is
   invoked by the "batch=stitchmode/x/y" option, and is called
   with the 'x' and 'y' parameters
*/

void make_mig(unsigned int x_mult, unsigned int y_mult)
{
    validate_input_gifs(x_mult, y_mult);

    std::string gif_out{"fractmig.gif"};
    std::printf("\n"
                " Generating multi-image GIF file %s using %u X and %u Y components\n"
                "\n",
        gif_out.c_str(), x_mult, y_mult);
    const std::filesystem::path path{get_checked_save_path(WriteFile::IMAGE, gif_out)};
    assert(!path.empty());

    write_tiled_images_gif(path, x_mult, y_mult);
    delete_tile_gifs(x_mult, y_mult);
    std::printf("File %s has been created (and its component files deleted)\n", gif_out.c_str());
}

} // namespace id::io
