// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/make_mig.h"

#include "engine/spindac.h"
#include "io/check_write_file.h"
#include "io/library.h"

#include <fmt/format.h>
#include <gif_lib.h>

#include <array> // std::size
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>

using namespace id::engine;

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

[[maybe_unused]] static InputGif read_input_gif(const std::filesystem::path &path)
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
        std::printf("MIG output dimensions %llu x %llu exceed GIF limits!\n", width, height);
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

[[maybe_unused]] static OutputGif create_output_gif(
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

static bool same_image(const SavedImage &lhs, const SavedImage &rhs)
{
    return lhs.ImageDesc.Left == rhs.ImageDesc.Left && lhs.ImageDesc.Top == rhs.ImageDesc.Top &&
        lhs.ImageDesc.Width == rhs.ImageDesc.Width && lhs.ImageDesc.Height == rhs.ImageDesc.Height &&
        lhs.ImageDesc.Interlace == rhs.ImageDesc.Interlace &&
        same_color_map(lhs.ImageDesc.ColorMap, rhs.ImageDesc.ColorMap) &&
        std::memcmp(lhs.RasterBits, rhs.RasterBits, raster_size(lhs)) == 0;
}

static bool same_image_at_offset(const SavedImage &lhs, const SavedImage &rhs, const int left, const int top)
{
    return lhs.ImageDesc.Left == rhs.ImageDesc.Left + left && lhs.ImageDesc.Top == rhs.ImageDesc.Top + top &&
        lhs.ImageDesc.Width == rhs.ImageDesc.Width && lhs.ImageDesc.Height == rhs.ImageDesc.Height &&
        lhs.ImageDesc.Interlace == rhs.ImageDesc.Interlace &&
        same_color_map(lhs.ImageDesc.ColorMap, rhs.ImageDesc.ColorMap) &&
        std::memcmp(lhs.RasterBits, rhs.RasterBits, raster_size(lhs)) == 0;
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

static void copy_saved_image_at_offset(GifFileType *output, const SavedImage &image, const int left, const int top)
{
    SavedImage *saved_image{copy_saved_image(output, image)};
    saved_image->ImageDesc.Left += left;
    saved_image->ImageDesc.Top += top;
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
            const int left{static_cast<int>(x_step) * input_metadata.width};
            const int top{static_cast<int>(y_step) * input_metadata.height};
            for (int i = 0; i < input.get()->ImageCount; i++)
            {
                if (!same_image_at_offset(output.get()->SavedImages[image_number++], input.get()->SavedImages[i], left, top))
                {
                    std::printf("Output file %s failed GIF offset verification!\n", path.filename().string().c_str());
                    std::exit(1);
                }
            }
        }
    }
}

[[maybe_unused]] static void write_tiled_images_gif(
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
            const int left{static_cast<int>(x_step) * input_metadata.width};
            const int top{static_cast<int>(y_step) * input_metadata.height};
            for (int i = 0; i < input.get()->ImageCount; i++)
            {
                copy_saved_image_at_offset(output.get(), input.get()->SavedImages[i], left, top);
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

    std::uint16_t x_res;
    std::uint16_t y_res;
    std::uint16_t x_tot;
    std::uint16_t y_tot;
    std::uint16_t x_loc;
    std::uint16_t y_loc;
    unsigned int i;
    std::string gif_in;
    unsigned char *temp;

    int error_flag = 0;                          // no errors so
    int input_error_flag = 0;
    unsigned int all_i_tbl = 0;
    unsigned int all_y_res = all_i_tbl;
    unsigned int all_x_res = all_y_res;
    std::FILE *in = nullptr;
    std::FILE *out = in;

    std::string gif_out{"fractmig.gif"};

    temp = &g_old_dac_box[0][0];                 // a safe place for our temp data

    // process each input image, one at a time
    for (unsigned y_step = 0U; y_step < y_mult; y_step++)
    {
        for (unsigned x_step = 0U; x_step < x_mult; x_step++)
        {
            if (x_step == 0 && y_step == 0)          // first time through?
            {
                std::printf("\n"
                            " Generating multi-image GIF file %s using %u X and %u Y components\n"
                            "\n",
                    gif_out.c_str(), x_mult, y_mult);
                // attempt to create the output file
                const std::filesystem::path path{get_checked_save_path(WriteFile::IMAGE, gif_out)};
                assert(!path.empty());
                out = std::fopen(path.string().c_str(), "wb");
                if (out == nullptr)
                {
                    std::printf("Cannot create output file %s!\n", gif_out.c_str());
                    std::exit(1);
                }
            }

            gif_in = fmt::format("frmig_{:c}{:c}.gif", par_key(x_step), par_key(y_step));
            const std::filesystem::path path{find_file(ReadFile::IMAGE, gif_in)};
            if (path.empty())
            {
                fmt::print("Can't locate file {:s}!\n", gif_in);
                std::exit(1);
            }
            in = std::fopen(path.string().c_str(), "rb");
            if (in == nullptr)
            {
                std::printf("Can't open file %s!\n", gif_in.c_str());
                std::exit(1);
            }

            // (read, but only copy this if it's the first time through)
            if (std::fread(temp, 13, 1, in) != 1)   // read the header and LDS
            {
                input_error_flag = 1;
            }
            std::memcpy(&x_res, &temp[6], sizeof(x_res));     // X-resolution
            std::memcpy(&y_res, &temp[8], sizeof(y_res));     // Y-resolution

            if (x_step == 0 && y_step == 0)  // first time through?
            {
                all_x_res = x_res;             // save the "master" resolution
                all_y_res = y_res;
                x_tot = x_res * x_mult;        // adjust the image size
                y_tot = y_res * y_mult;
                std::memcpy(&temp[6], &x_tot, sizeof(x_tot));
                std::memcpy(&temp[8], &y_tot, sizeof(y_tot));
                temp[12] = 0; // reserved
                if (std::fwrite(temp, 13, 1, out) != 1)     // write out the header
                {
                    error_flag = 1;
                }
            }                           // end of first-time-through

            unsigned char i_char = static_cast<char>(temp[10] & 0x07);        // find the color table size
            unsigned int i_tbl = 1 << ++i_char;
            i_char = static_cast<char>(temp[10] & 0x80);        // is there a global color table?
            if (x_step == 0 && y_step == 0)   // first time through?
            {
                all_i_tbl = i_tbl;             // save the color table size
            }
            if (i_char != 0)                // yup
            {
                // (read, but only copy this if it's the first time through)
                if (std::fread(temp, 3*i_tbl, 1, in) != 1)    // read the global color table
                {
                    input_error_flag = 2;
                }
                if (x_step == 0 && y_step == 0)       // first time through?
                {
                    if (std::fwrite(temp, 3*i_tbl, 1, out) != 1)     // write out the GCT
                    {
                        error_flag = 2;
                    }
                }
            }

            if (x_res != all_x_res || y_res != all_y_res || i_tbl != all_i_tbl)
            {
                // Oops - our pieces don't match
                std::printf("File %s doesn't have the same resolution as its predecessors!\n", gif_in.c_str());
                std::exit(1);
            }

            while (true)                       // process each information block
            {
                std::memset(temp, 0, 10);
                if (std::fread(temp, 1, 1, in) != 1)    // read the block identifier
                {
                    input_error_flag = 3;
                }

                if (temp[0] == 0x2c)           // image descriptor block
                {
                    if (std::fread(&temp[1], 9, 1, in) != 1)    // read the Image Descriptor
                    {
                        input_error_flag = 4;
                    }
                    std::memcpy(&x_loc, &temp[1], sizeof(x_loc)); // X-location
                    std::memcpy(&y_loc, &temp[3], sizeof(y_loc)); // Y-location
                    x_loc += x_step * x_res;     // adjust the locations
                    y_loc += y_step * y_res;
                    std::memcpy(&temp[1], &x_loc, sizeof(x_loc));
                    std::memcpy(&temp[3], &y_loc, sizeof(y_loc));
                    if (std::fwrite(temp, 10, 1, out) != 1)     // write out the Image Descriptor
                    {
                        error_flag = 4;
                    }

                    i_char = static_cast<char>(temp[9] & 0x80);     // is there a local color table?
                    if (i_char != 0)            // yup
                    {
                        if (std::fread(temp, 3*i_tbl, 1, in) != 1)       // read the local color table
                        {
                            input_error_flag = 5;
                        }
                        if (std::fwrite(temp, 3*i_tbl, 1, out) != 1)     // write out the LCT
                        {
                            error_flag = 5;
                        }
                    }

                    if (std::fread(temp, 1, 1, in) != 1)        // LZH table size
                    {
                        input_error_flag = 6;
                    }
                    if (std::fwrite(temp, 1, 1, out) != 1)
                    {
                        error_flag = 6;
                    }
                    while (true)
                    {
                        if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            input_error_flag = 7;
                        }
                        if (std::fwrite(temp, 1, 1, out) != 1)
                        {
                            error_flag = 7;
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // LZH data block
                        {
                            input_error_flag = 8;
                        }
                        if (std::fwrite(temp, i, 1, out) != 1)
                        {
                            error_flag = 8;
                        }
                    }
                }

                if (temp[0] == 0x21)           // extension block
                {
                    // (read, but only copy this if it's the last time through)
                    if (std::fread(&temp[2], 1, 1, in) != 1)    // read the block type
                    {
                        input_error_flag = 9;
                    }
                    if (x_step == x_mult-1 && y_step == y_mult-1)
                    {
                        if (std::fwrite(temp, 2, 1, out) != 1)
                        {
                            error_flag = 9;
                        }
                    }
                    while (true)
                    {
                        if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            input_error_flag = 10;
                        }
                        if (x_step == x_mult-1 && y_step == y_mult-1)
                        {
                            if (std::fwrite(temp, 1, 1, out) != 1)
                            {
                                error_flag = 10;
                            }
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // data block
                        {
                            input_error_flag = 11;
                        }
                        if (x_step == x_mult-1 && y_step == y_mult-1)
                        {
                            if (std::fwrite(temp, i, 1, out) != 1)
                            {
                                error_flag = 11;
                            }
                        }
                    }
                }

                if (temp[0] == 0x3b)           // end-of-stream indicator
                {
                    break;                      // done with this file
                }

                if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
                {
                    break;
                }
            }
            std::fclose(in);                     // done with an input GIF

            if (error_flag != 0 || input_error_flag != 0)      // oops - did something go wrong?
            {
                break;
            }
        }

        if (error_flag != 0 || input_error_flag != 0)  // oops - did something go wrong?
        {
            break;
        }
    }

    temp[0] = 0x3b;                 // end-of-stream indicator
    if (std::fwrite(temp, 1, 1, out) != 1)
    {
        error_flag = 12;
    }
    std::fclose(out);                    // done with the output GIF

    if (input_error_flag != 0)       // uh-oh - something failed
    {
        std::printf("\007 Process failed = early EOF on input file %s\n", gif_in.c_str());
    }

    if (error_flag != 0)            // uh-oh - something failed
    {
        std::printf("\007 Process failed = out of disk space?\n");
    }

    // now delete each input image, one at a time
    if (error_flag == 0 && input_error_flag == 0)
    {
        for (unsigned y_step = 0U; y_step < y_mult; y_step++)
        {
            for (unsigned x_step = 0U; x_step < x_mult; x_step++)
            {
                gif_in = fmt::format("frmig_{:c}{:c}.gif", par_key(x_step), par_key(y_step));
                std::filesystem::remove(get_save_path(WriteFile::IMAGE, gif_in));
            }
        }
    }

    // tell the world we're done
    if (error_flag == 0 && input_error_flag == 0)
    {
        std::printf("File %s has been created (and its component files deleted)\n", gif_out.c_str());
    }
}

} // namespace id::io
