// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include <io/GifWriter.h>

#include <io/gif_extensions.h>
#include <io/gif_file.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <ui/make_batch_file.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

using GifFilePtr = std::unique_ptr<GifFileType, void (*)(GifFileType *)>;

class FailingBuffer : public std::stringbuf
{
public:
    std::streamsize remaining{std::numeric_limits<std::streamsize>::max()};
    bool fail_sync{};
    int sync_count{};

protected:
    std::streamsize xsputn(const char *data, std::streamsize count) override;
    int sync() override;
};

std::streamsize FailingBuffer::xsputn(const char *data, const std::streamsize count)
{
    const std::streamsize written{std::stringbuf::xsputn(data, std::min(count, remaining))};
    remaining -= written;
    return written;
}

int FailingBuffer::sync()
{
    ++sync_count;
    return fail_sync ? -1 : 0;
}

GifWriter::Palette make_palette(const int count)
{
    GifWriter::Palette palette;
    for (int i = 0; i < count; ++i)
    {
        palette.push_back({static_cast<Byte>(i), static_cast<Byte>(255 - i), static_cast<Byte>(i * 37)});
    }
    return palette;
}

} // namespace

class TestGifWriter : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;
    GifFilePtr read_file() const;
    void expect_pixels(const GifFileType &gif, const std::vector<Byte> &expected, int width, int height) const;

    std::filesystem::path m_path;
    GifFileType m_extensions{};
};

void TestGifWriter::SetUp()
{
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_path = std::filesystem::temp_directory_path() / ("id-GifWriter-" + name + ".gif");
    std::filesystem::remove(m_path);
}

void TestGifWriter::TearDown()
{
    GifFreeExtensions(&m_extensions.ExtensionBlockCount, &m_extensions.ExtensionBlocks);
    std::filesystem::remove(m_path);
}

GifFilePtr TestGifWriter::read_file() const
{
    int error{};
    GifFilePtr gif{DGifOpenFileName(m_path.string().c_str(), &error), [](GifFileType *input)
        {
            int close_error{};
            DGifCloseFile(input, &close_error);
        }};
    if (!gif || DGifSlurp(gif.get()) == GIF_ERROR)
    {
        throw std::runtime_error("Can't decode GIF writer test output");
    }
    return gif;
}

void TestGifWriter::expect_pixels(
    const GifFileType &gif, const std::vector<Byte> &expected, const int width, const int height) const
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
        expected, (std::vector<Byte>{image.RasterBits, image.RasterBits + static_cast<std::size_t>(width) * height}));
}

class TestGifWriterColors : public TestGifWriter, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterColors, writesPaletteHeaderAndScanlinesWithoutChangingInput)
{
    const int colors{GetParam()};
    const GifWriter::Palette palette{make_palette(colors)};
    GifWriter writer{m_path, {8, 3, 8, 1, 49}, palette};
    ASSERT_EQ(0, writer.error());
    std::vector<Byte> expected;
    for (int row = 0; row < 3; ++row)
    {
        std::vector<Byte> pixels;
        for (int col = 0; col < 8; ++col)
        {
            pixels.push_back(static_cast<Byte>((row * 17 + col * 31) % colors));
        }
        const std::vector<Byte> original{pixels};
        ASSERT_TRUE(writer.write_pixels(pixels));
        EXPECT_EQ(original, pixels);
        expected.insert(expected.end(), original.begin(), original.end());
    }
    ASSERT_TRUE(writer.finish());

    const GifFilePtr gif{read_file()};
    expect_pixels(*gif, expected, 8, 3);
    EXPECT_EQ(8, gif->SColorResolution);
    EXPECT_EQ(1, gif->SBackGroundColor);
    EXPECT_EQ(49, gif->AspectByte);
    ASSERT_NE(nullptr, gif->SColorMap);
    ASSERT_EQ(colors, gif->SColorMap->ColorCount);
    for (int i = 0; i < colors; ++i)
    {
        const GifColorType &color{gif->SColorMap->Colors[i]};
        EXPECT_EQ(palette[i], (std::array<Byte, 3>{color.Red, color.Green, color.Blue}));
    }
    std::ifstream input{m_path, std::ios::binary};
    std::array<char, 6> signature{};
    input.read(signature.data(), signature.size());
    EXPECT_EQ("GIF89a", (std::string{signature.data(), signature.size()}));
}

INSTANTIATE_TEST_SUITE_P(Palettes, TestGifWriterColors, Values(2, 4, 16, 256));

TEST_F(TestGifWriter, writesTwoHalfRowsWithoutKnowingPotentialStoragePolicy)
{
    GifWriter writer{m_path, {6, 2}, make_palette(256)};

    ASSERT_TRUE(writer.write_pixels({1, 2, 3}));
    ASSERT_TRUE(writer.write_pixels({201, 202, 203}));
    ASSERT_TRUE(writer.write_pixels({4, 5, 6}));
    ASSERT_TRUE(writer.write_pixels({204, 205, 206}));
    ASSERT_TRUE(writer.finish());

    const GifFilePtr gif{read_file()};
    expect_pixels(*gif, {1, 2, 3, 201, 202, 203, 4, 5, 6, 204, 205, 206}, 6, 2);
}

TEST_F(TestGifWriter, emitsSharedExtensionsWithOriginalGroupingAndEmptyRecords)
{
    ValueSaver saved_width{g_file_x_dots};
    ValueSaver saved_height{g_file_y_dots};
    ValueSaver saved_colors{g_file_colors};
    ValueSaver saved_aspect{g_file_aspect_ratio};
    ValueSaver saved_make_parameter_file{g_make_parameter_file, false};
    const std::vector<Byte> resume(511, 0xA5);
    put_resume_info(&m_extensions, resume);
    put_extended_param_info(&m_extensions, {});
    put_ranges_info(&m_extensions, {-1, 128, 32767});
    FractalInfo info{};
    std::copy_n(INFO_ID, sizeof(info.info_id), info.info_id);
    info.info_version = FRACTAL_INFO_VERSION;
    info.iterations = 150;
    info.x_dots = 2;
    info.y_dots = 2;
    put_fractal_info(&m_extensions, info);
    GifWriter writer{m_path, {2, 2}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    ASSERT_TRUE(writer.write_pixels({1, 0}));

    ASSERT_TRUE(writer.write_extensions(m_extensions));
    ASSERT_TRUE(writer.finish());

    const GifFilePtr gif{read_file()};
    expect_pixels(*gif, {0, 1, 1, 0}, 2, 2);
    ASSERT_EQ(m_extensions.ExtensionBlockCount, gif->ExtensionBlockCount);
    for (int i = 0; i < m_extensions.ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &expected{m_extensions.ExtensionBlocks[i]};
        const ExtensionBlock &actual{gif->ExtensionBlocks[i]};
        EXPECT_EQ(expected.Function, actual.Function);
        ASSERT_EQ(expected.ByteCount, actual.ByteCount);
        EXPECT_EQ((std::vector<Byte>{expected.Bytes, expected.Bytes + expected.ByteCount}),
            (std::vector<Byte>{actual.Bytes, actual.Bytes + actual.ByteCount}));
    }
    EXPECT_EQ(resume, get_resume_info(gif.get()));
    EXPECT_TRUE(has_gif_extension(gif.get(), "fractint005"));
    EXPECT_TRUE(get_extended_param_info(gif.get()).empty());
    FractalInfo loaded{};
    ExtBlock2 block2{};
    ExtBlock3 block3{};
    ExtBlock4 block4{};
    ExtBlock5 block5{};
    ExtBlock6 block6{};
    ExtBlock7 block7{};
    ASSERT_FALSE(find_fractal_info(m_path.string(), &loaded, &block2, &block3, &block4, &block5, &block6, &block7));
    EXPECT_EQ(150, loaded.iterations);
    EXPECT_TRUE(block2.got_data);
    EXPECT_EQ(resume, block2.resume_data);
    EXPECT_TRUE(block4.got_data);
    EXPECT_EQ((std::vector<int>{-1, 128, 32767}), block4.range_data);
    EXPECT_TRUE(block5.got_data);
    EXPECT_TRUE(block5.apm_data.empty());
}

TEST_F(TestGifWriter, finishIsIdempotentAndLeavesBorrowedStreamOpen)
{
    FailingBuffer buffer;
    std::ostream output{&buffer};
    std::string complete;
    {
        GifWriter writer{output, {2, 1}, make_palette(2)};
        ASSERT_TRUE(writer.write_pixels({0, 1}));
        ASSERT_TRUE(writer.finish());
        complete = buffer.str();
        EXPECT_TRUE(writer.finish());
        EXPECT_EQ(complete, buffer.str());
        EXPECT_EQ(1, buffer.sync_count);
        EXPECT_THROW(writer.write_pixels({0, 1}), std::logic_error);
        EXPECT_THROW(writer.write_extensions(m_extensions), std::logic_error);
    }
    EXPECT_EQ(complete, buffer.str());
    EXPECT_EQ(1, buffer.sync_count);
    output << 'X';
    EXPECT_EQ(complete + 'X', buffer.str());
}

TEST_F(TestGifWriter, releasesIncompleteFileOnDestruction)
{
    {
        GifWriter writer{m_path, {2, 2}, make_palette(2)};
        ASSERT_TRUE(writer.write_pixels({0, 1}));
    }

    EXPECT_TRUE(std::filesystem::remove(m_path));
}

TEST_F(TestGifWriter, rejectsInvalidCallSequencesWithoutAdvancingRaster)
{
    std::ostringstream output;
    GifWriter writer{output, {2, 2}, make_palette(2)};

    EXPECT_THROW(writer.finish(), std::logic_error);
    EXPECT_THROW(writer.write_extensions(m_extensions), std::logic_error);
    EXPECT_THROW(writer.write_pixels({}), std::logic_error);
    EXPECT_THROW(writer.write_pixels({0, 1, 0}), std::logic_error);
    EXPECT_THROW(writer.write_pixels({0, 2}), std::logic_error);
    ASSERT_TRUE(writer.write_pixels({0}));
    EXPECT_THROW(writer.write_pixels({1, 0}), std::logic_error);
    ASSERT_TRUE(writer.write_pixels({1}));
    ASSERT_TRUE(writer.write_pixels({1, 0}));
    EXPECT_THROW(writer.write_pixels({0}), std::logic_error);
    EXPECT_TRUE(writer.write_extensions(m_extensions));
    EXPECT_TRUE(writer.finish());
}

TEST_F(TestGifWriter, rejectsMalformedExtensionBlocksBeforeWritingThem)
{
    std::ostringstream output;
    GifWriter writer{output, {2, 1}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    const std::string raster{output.str()};
    Byte bytes[256]{};
    ExtensionBlock block{11, bytes, CONTINUE_EXT_FUNC_CODE};
    // This block is borrowed, not allocated by giflib.
    GifFileType malformed{};
    malformed.ExtensionBlockCount = 1;
    malformed.ExtensionBlocks = &block;

    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    block.Function = APPLICATION_EXT_FUNC_CODE;
    block.ByteCount = 256;
    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    block.ByteCount = 0;
    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    block.ByteCount = 11;
    block.Bytes = nullptr;
    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    malformed.ExtensionBlockCount = -1;
    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    malformed.ExtensionBlockCount = 1;
    malformed.ExtensionBlocks = nullptr;
    EXPECT_THROW(writer.write_extensions(malformed), std::logic_error);
    EXPECT_EQ(raster, output.str());
    EXPECT_TRUE(writer.finish());
}

TEST_F(TestGifWriter, reportsOpenFailureWithoutCreatingMissingDirectories)
{
    GifWriter writer{m_path / "missing.gif", {2, 1}, make_palette(2)};

    EXPECT_EQ(E_GIF_ERR_OPEN_FAILED, writer.error());
    EXPECT_FALSE(writer.write_pixels({0, 1}));
    EXPECT_FALSE(writer.write_extensions(m_extensions));
    EXPECT_FALSE(writer.finish());
    EXPECT_FALSE(std::filesystem::exists(m_path));
}

class TestGifWriterHeaderFailure : public TestGifWriter, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterHeaderFailure, detectsShortWritesIncludingUncheckedDescriptorFields)
{
    FailingBuffer buffer;
    buffer.remaining = GetParam();
    std::ostream output{&buffer};
    GifWriter writer{output, {2, 1}, make_palette(2)};

    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    EXPECT_FALSE(writer.finish());
}

INSTANTIATE_TEST_SUITE_P(ByteLimits, TestGifWriterHeaderFailure, Values(0, 6, 8, 13, 19, 20, 29));

TEST_F(TestGifWriter, reportsCompressedRowFailureAndRetainsFirstError)
{
    FailingBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {2, 2}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    buffer.remaining = 0;

    EXPECT_FALSE(writer.write_pixels({1, 0}));
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    EXPECT_FALSE(writer.finish());
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
}

class TestGifWriterExtensionFailure : public TestGifWriter, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterExtensionFailure, detectsShortLeaderPayloadAndTerminatorWrites)
{
    put_resume_info(&m_extensions, std::vector<Byte>(511, 0xA5));
    FailingBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {2, 1}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    buffer.remaining = GetParam();

    EXPECT_FALSE(writer.write_extensions(m_extensions));
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    EXPECT_FALSE(writer.finish());
}

INSTANTIATE_TEST_SUITE_P(ByteLimits, TestGifWriterExtensionFailure, Values(0, 2, 3, 14, 15, 270, 528));

TEST_F(TestGifWriter, detectsTrailerFailureEvenWhenGiflibCloseReportsSuccess)
{
    FailingBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {2, 1}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    buffer.remaining = 0;

    EXPECT_FALSE(writer.finish());
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    const std::string incomplete{buffer.str()};
    EXPECT_FALSE(writer.finish());
    EXPECT_EQ(incomplete, buffer.str());
}

TEST_F(TestGifWriter, reportsFinalOutputFlushFailure)
{
    FailingBuffer buffer;
    std::ostream output{&buffer};
    GifWriter writer{output, {2, 1}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    buffer.fail_sync = true;

    EXPECT_FALSE(writer.finish());
    EXPECT_EQ(E_GIF_ERR_CLOSE_FAILED, writer.error());
    EXPECT_EQ(1, buffer.sync_count);
    EXPECT_FALSE(writer.finish());
    EXPECT_EQ(1, buffer.sync_count);
}

TEST_F(TestGifWriter, containsStreamExceptionsAtTheGiflibCallbackBoundary)
{
    FailingBuffer buffer;
    buffer.remaining = 0;
    std::ostream output{&buffer};
    output.exceptions(std::ios::badbit | std::ios::failbit);
    GifWriter writer{output, {2, 1}, make_palette(2)};

    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, writer.error());
    EXPECT_FALSE(writer.finish());
}

class TestGifWriterSize : public TestGifWriter, public WithParamInterface<std::pair<int, int>>
{
};

TEST_P(TestGifWriterSize, supportsUnsignedDimensionBoundaries)
{
    const int width{GetParam().first};
    const int height{GetParam().second};
    GifWriter writer{m_path, {width, height}, make_palette(2)};
    std::vector<Byte> row(width);
    for (int col = 0; col < width; ++col)
    {
        row[col] = static_cast<Byte>(col % 2);
    }
    std::vector<Byte> expected;
    for (int y = 0; y < height; ++y)
    {
        ASSERT_TRUE(writer.write_pixels(row));
        expected.insert(expected.end(), row.begin(), row.end());
    }
    ASSERT_TRUE(writer.finish());

    const GifFilePtr gif{read_file()};
    expect_pixels(*gif, expected, width, height);
}

INSTANTIATE_TEST_SUITE_P(UnsignedBounds, TestGifWriterSize,
    Values(std::pair{32768, 1}, std::pair{65535, 1}, std::pair{1, 32768}, std::pair{1, 65535}));

TEST_F(TestGifWriter, rejectsInvalidScreensAndPalettesBeforeOpeningOutput)
{
    for (const GifWriter::Screen screen : {GifWriter::Screen{0, 1}, {-1, 1}, {1, 0}, {1, -1}, {65536, 1}, {1, 65536},
             {2, 1, 0}, {2, 1, 9}, {2, 1, 8, -1}, {2, 1, 8, 2}})
    {
        EXPECT_THROW((GifWriter{m_path, screen, make_palette(2)}), std::logic_error);
        EXPECT_FALSE(std::filesystem::exists(m_path));
    }
    for (const int count : {0, 1, 3, 257})
    {
        EXPECT_THROW((GifWriter{m_path, {2, 1}, make_palette(count)}), std::logic_error);
        EXPECT_FALSE(std::filesystem::exists(m_path));
    }
}

} // namespace id::test
