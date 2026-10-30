// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include <io/GifWriter.h>

#include "FailingOutputBuffer.h"
#include "FindFractalInfoState.h"
#include "FindFractalInfoTestSupport.h"
#include "GifReadSupport.h"

#include <io/gif_extensions.h>
#include <io/gif_file.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace id::io;
using namespace testing;

namespace id::test
{

namespace
{

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

    std::filesystem::path m_path;
    GifFileType m_extensions{};
};

void TestGifWriter::SetUp()
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{std::string{info->test_suite_name()} + "." + info->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_path = std::filesystem::temp_directory_path() / ("id-GifWriter-" + name + ".gif");
    std::filesystem::remove(m_path);
}

void TestGifWriter::TearDown()
{
    GifFreeExtensions(&m_extensions.ExtensionBlockCount, &m_extensions.ExtensionBlocks);
    std::filesystem::remove(m_path);
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

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, expected, 8, 3);
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

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, {1, 2, 3, 201, 202, 203, 4, 5, 6, 204, 205, 206}, 6, 2);
}

TEST_F(TestGifWriter, emitsSharedExtensionsWithOriginalGroupingAndEmptyRecords)
{
    FindFractalInfoState saved_state;
    const std::vector<Byte> resume(511, 0xA5);
    const std::vector<char> parameters(396, 'a');
    put_resume_info(&m_extensions, resume);
    put_extended_param_info(&m_extensions, parameters);
    put_ranges_info(&m_extensions, {});
    FractalInfo info{};
    std::copy_n(INFO_ID, sizeof(info.info_id), info.info_id);
    info.info_version = FRACTAL_INFO_VERSION;
    info.iterations = 150;
    info.x_dots = 2;
    info.y_dots = 2;
    info.bf_length = 12;
    info.bf_math = 1;
    put_fractal_info(&m_extensions, info);
    GifWriter writer{m_path, {2, 2}, make_palette(2)};
    ASSERT_TRUE(writer.write_pixels({0, 1}));
    ASSERT_TRUE(writer.write_pixels({1, 0}));

    ASSERT_TRUE(writer.write_extensions(m_extensions));
    ASSERT_TRUE(writer.finish());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, {0, 1, 1, 0}, 2, 2);
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
    EXPECT_EQ(parameters, get_extended_param_info(gif.get()));
    FindFractalInfoResult loaded;
    ASSERT_FALSE(loaded.read(m_path));
    EXPECT_EQ(150, loaded.info.iterations);
    EXPECT_TRUE(loaded.resume.got_data);
    EXPECT_EQ(resume, loaded.resume.resume_data);
    EXPECT_TRUE(loaded.ranges.got_data);
    EXPECT_TRUE(loaded.ranges.range_data.empty());
    EXPECT_TRUE(loaded.parameters.got_data);
    EXPECT_EQ(parameters, loaded.parameters.apm_data);
}

class TestFinishedGifWriter : public TestGifWriter
{
protected:
    void SetUp() override;

    FailingOutputBuffer m_buffer;
    std::ostream m_output{&m_buffer};
    std::unique_ptr<GifWriter> m_writer;
    std::string m_complete;
};

void TestFinishedGifWriter::SetUp()
{
    TestGifWriter::SetUp();
    m_writer = std::make_unique<GifWriter>(m_output, GifWriter::Screen{2, 1}, make_palette(2));
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
    ASSERT_TRUE(m_writer->finish());
    m_complete = m_buffer.str();
}

TEST_F(TestFinishedGifWriter, finishIsIdempotent)
{
    EXPECT_TRUE(m_writer->finish());

    EXPECT_EQ(m_complete, m_buffer.str());
    EXPECT_EQ(1, m_buffer.sync_count);
    m_writer.reset();
    EXPECT_EQ(m_complete, m_buffer.str());
    EXPECT_EQ(1, m_buffer.sync_count);
}

TEST_F(TestFinishedGifWriter, destructionLeavesBorrowedStreamOpen)
{
    m_writer.reset();

    EXPECT_EQ(m_complete, m_buffer.str());
    EXPECT_EQ(1, m_buffer.sync_count);
    m_output << 'X';
    EXPECT_EQ(m_complete + 'X', m_buffer.str());
}

TEST_F(TestFinishedGifWriter, rejectsPixelsAfterFinish)
{
    EXPECT_THROW(m_writer->write_pixels({0, 1}), std::logic_error);
    m_writer.reset();

    EXPECT_EQ(m_complete, m_buffer.str());
    EXPECT_EQ(1, m_buffer.sync_count);
}

TEST_F(TestFinishedGifWriter, rejectsExtensionsAfterFinish)
{
    EXPECT_THROW(m_writer->write_extensions(m_extensions), std::logic_error);
    m_writer.reset();

    EXPECT_EQ(m_complete, m_buffer.str());
    EXPECT_EQ(1, m_buffer.sync_count);
}

TEST_F(TestGifWriter, releasesIncompleteFileOnDestruction)
{
    {
        GifWriter writer{m_path, {2, 2}, make_palette(2)};
        ASSERT_TRUE(writer.write_pixels({0, 1}));
    }

    EXPECT_TRUE(std::filesystem::remove(m_path));
}

class TestGifWriterSequence : public TestGifWriter
{
protected:
    void SetUp() override;
    void complete_raster();
    void finish_output();

    std::ostringstream m_output;
    std::unique_ptr<GifWriter> m_writer;
};

void TestGifWriterSequence::SetUp()
{
    TestGifWriter::SetUp();
    m_writer = std::make_unique<GifWriter>(m_output, GifWriter::Screen{2, 2}, make_palette(2));
}

void TestGifWriterSequence::complete_raster()
{
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
    ASSERT_TRUE(m_writer->write_pixels({1, 0}));
    finish_output();
}

void TestGifWriterSequence::finish_output()
{
    EXPECT_TRUE(m_writer->write_extensions(m_extensions));
    EXPECT_TRUE(m_writer->finish());
}

TEST_F(TestGifWriterSequence, rejectsPrematureFinishWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->finish(), std::logic_error);

    complete_raster();
}

TEST_F(TestGifWriterSequence, rejectsPrematureExtensionsWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->write_extensions(m_extensions), std::logic_error);

    complete_raster();
}

TEST_F(TestGifWriterSequence, rejectsEmptyPixelsWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->write_pixels({}), std::logic_error);

    complete_raster();
}

TEST_F(TestGifWriterSequence, rejectsRowOverflowWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->write_pixels({0, 1, 0}), std::logic_error);

    complete_raster();
}

TEST_F(TestGifWriterSequence, rejectsInvalidPaletteIndexWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->write_pixels({0, 2}), std::logic_error);

    complete_raster();
}

class TestGifWriterPartialRow : public TestGifWriterSequence
{
protected:
    void SetUp() override;
};

void TestGifWriterPartialRow::SetUp()
{
    TestGifWriterSequence::SetUp();
    ASSERT_TRUE(m_writer->write_pixels({0}));
}

TEST_F(TestGifWriterPartialRow, rejectsRowOverflowWithoutAdvancingRaster)
{
    EXPECT_THROW(m_writer->write_pixels({1, 0}), std::logic_error);

    ASSERT_TRUE(m_writer->write_pixels({1}));
    ASSERT_TRUE(m_writer->write_pixels({1, 0}));
    finish_output();
}

class TestGifWriterCompleteRaster : public TestGifWriterSequence
{
protected:
    void SetUp() override;
};

void TestGifWriterCompleteRaster::SetUp()
{
    TestGifWriterSequence::SetUp();
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
    ASSERT_TRUE(m_writer->write_pixels({1, 0}));
}

TEST_F(TestGifWriterCompleteRaster, rejectsPixelsBeyondCompletedRaster)
{
    EXPECT_THROW(m_writer->write_pixels({0}), std::logic_error);

    finish_output();
}

struct MalformedExtensionCase
{
    const char *name;
    int function;
    int bytes;
    bool missing_bytes;
    int blocks;
    bool missing_blocks;
};

void PrintTo(const MalformedExtensionCase &value, std::ostream *str)
{
    *str << value.name;
}

class TestGifWriterMalformedExtension : public TestGifWriter, public WithParamInterface<MalformedExtensionCase>
{
protected:
    void SetUp() override;

    std::ostringstream m_output;
    Byte m_bytes[256]{};
    ExtensionBlock m_block{};
    // This block is borrowed, not allocated by giflib.
    GifFileType m_malformed{};
    std::string m_raster;
    std::unique_ptr<GifWriter> m_writer;
};

void TestGifWriterMalformedExtension::SetUp()
{
    TestGifWriter::SetUp();
    m_writer = std::make_unique<GifWriter>(m_output, GifWriter::Screen{2, 1}, make_palette(2));
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
    m_raster = m_output.str();
    const MalformedExtensionCase &input{GetParam()};
    m_block = {input.bytes, input.missing_bytes ? nullptr : m_bytes, input.function};
    m_malformed.ExtensionBlockCount = input.blocks;
    m_malformed.ExtensionBlocks = input.missing_blocks ? nullptr : &m_block;
}

TEST_P(TestGifWriterMalformedExtension, rejectsMalformedBlocksBeforeWritingThem)
{
    EXPECT_THROW(m_writer->write_extensions(m_malformed), std::logic_error);

    EXPECT_EQ(m_raster, m_output.str());
    EXPECT_TRUE(m_writer->finish());
}

INSTANTIATE_TEST_SUITE_P(ExtensionInputs, TestGifWriterMalformedExtension,
    Values(MalformedExtensionCase{"OrphanContinuation", CONTINUE_EXT_FUNC_CODE, 11, false, 1, false},
        MalformedExtensionCase{"OversizedBlock", APPLICATION_EXT_FUNC_CODE, 256, false, 1, false},
        MalformedExtensionCase{"EmptyBlock", APPLICATION_EXT_FUNC_CODE, 0, false, 1, false},
        MalformedExtensionCase{"MissingBytes", APPLICATION_EXT_FUNC_CODE, 11, true, 1, false},
        MalformedExtensionCase{"NegativeCount", APPLICATION_EXT_FUNC_CODE, 11, true, -1, false},
        MalformedExtensionCase{"MissingBlocks", APPLICATION_EXT_FUNC_CODE, 11, true, 1, true}),
    [](const TestParamInfo<MalformedExtensionCase> &info) { return info.param.name; });

TEST_F(TestGifWriter, reportsOpenFailureWithoutCreatingMissingDirectories)
{
    GifWriter writer{m_path / "missing.gif", {2, 1}, make_palette(2)};

    EXPECT_EQ(E_GIF_ERR_OPEN_FAILED, writer.error());
    EXPECT_FALSE(std::filesystem::exists(m_path));
}

class TestFailedOpenGifWriter : public TestGifWriter
{
protected:
    void SetUp() override;

    std::unique_ptr<GifWriter> m_writer;
};

void TestFailedOpenGifWriter::SetUp()
{
    TestGifWriter::SetUp();
    m_writer = std::make_unique<GifWriter>(m_path / "missing.gif", GifWriter::Screen{2, 1}, make_palette(2));
    ASSERT_EQ(E_GIF_ERR_OPEN_FAILED, m_writer->error());
}

TEST_F(TestFailedOpenGifWriter, pixelWriteReportsFailureWithoutCreatingOutput)
{
    EXPECT_FALSE(m_writer->write_pixels({0, 1}));

    EXPECT_FALSE(std::filesystem::exists(m_path));
}

TEST_F(TestFailedOpenGifWriter, extensionWriteReportsFailureWithoutCreatingOutput)
{
    EXPECT_FALSE(m_writer->write_extensions(m_extensions));

    EXPECT_FALSE(std::filesystem::exists(m_path));
}

TEST_F(TestFailedOpenGifWriter, finishReportsFailureWithoutCreatingOutput)
{
    EXPECT_FALSE(m_writer->finish());

    EXPECT_FALSE(std::filesystem::exists(m_path));
}

class GifWriterStreamTest : public TestGifWriter
{
protected:
    void open_writer(const GifWriter::Screen &screen);

    FailingOutputBuffer m_buffer;
    std::ostream m_output{&m_buffer};
    std::unique_ptr<GifWriter> m_writer;
};

void GifWriterStreamTest::open_writer(const GifWriter::Screen &screen)
{
    m_writer = std::make_unique<GifWriter>(m_output, screen, make_palette(2));
}

class TestGifWriterHeaderFailure : public GifWriterStreamTest, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestGifWriterHeaderFailure::SetUp()
{
    TestGifWriter::SetUp();
    m_buffer.remaining = GetParam();
    open_writer({2, 1});
}

TEST_P(TestGifWriterHeaderFailure, detectsShortWritesIncludingUncheckedDescriptorFields)
{
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
    EXPECT_FALSE(m_writer->finish());
}

INSTANTIATE_TEST_SUITE_P(ByteLimits, TestGifWriterHeaderFailure, Values(0, 6, 8, 13, 19, 20, 29));

class TestGifWriterRasterFailure : public GifWriterStreamTest
{
protected:
    void SetUp() override;
};

void TestGifWriterRasterFailure::SetUp()
{
    TestGifWriter::SetUp();
    open_writer({2, 2});
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
    m_buffer.remaining = 0;
}

TEST_F(TestGifWriterRasterFailure, reportsCompressedRowFailureAndRetainsFirstError)
{
    EXPECT_FALSE(m_writer->write_pixels({1, 0}));
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
    EXPECT_FALSE(m_writer->finish());
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
}

class GifWriterCompletedRowTest : public GifWriterStreamTest
{
protected:
    void SetUp() override;
};

void GifWriterCompletedRowTest::SetUp()
{
    TestGifWriter::SetUp();
    open_writer({2, 1});
    ASSERT_TRUE(m_writer->write_pixels({0, 1}));
}

class TestGifWriterExtensionFailure : public GifWriterCompletedRowTest, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestGifWriterExtensionFailure::SetUp()
{
    GifWriterCompletedRowTest::SetUp();
    put_resume_info(&m_extensions, std::vector<Byte>(511, 0xA5));
    m_buffer.remaining = GetParam();
}

TEST_P(TestGifWriterExtensionFailure, detectsShortLeaderPayloadAndTerminatorWrites)
{
    EXPECT_FALSE(m_writer->write_extensions(m_extensions));
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
    EXPECT_FALSE(m_writer->finish());
}

INSTANTIATE_TEST_SUITE_P(ByteLimits, TestGifWriterExtensionFailure, Values(0, 2, 3, 14, 15, 270, 528));

class TestGifWriterTrailerFailure : public GifWriterCompletedRowTest
{
protected:
    void SetUp() override;
};

void TestGifWriterTrailerFailure::SetUp()
{
    GifWriterCompletedRowTest::SetUp();
    m_buffer.remaining = 0;
}

TEST_F(TestGifWriterTrailerFailure, detectsTrailerFailureEvenWhenGiflibCloseReportsSuccess)
{
    EXPECT_FALSE(m_writer->finish());
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
    const std::string incomplete{m_buffer.str()};
    EXPECT_FALSE(m_writer->finish());
    EXPECT_EQ(incomplete, m_buffer.str());
}

class TestGifWriterFlushFailure : public GifWriterCompletedRowTest
{
protected:
    void SetUp() override;
};

void TestGifWriterFlushFailure::SetUp()
{
    GifWriterCompletedRowTest::SetUp();
    m_buffer.fail_sync = true;
}

TEST_F(TestGifWriterFlushFailure, reportsFinalOutputFlushFailure)
{
    EXPECT_FALSE(m_writer->finish());
    EXPECT_EQ(E_GIF_ERR_CLOSE_FAILED, m_writer->error());
    EXPECT_EQ(1, m_buffer.sync_count);
    EXPECT_FALSE(m_writer->finish());
    EXPECT_EQ(1, m_buffer.sync_count);
}

class TestGifWriterCallbackFailure : public GifWriterStreamTest
{
protected:
    void SetUp() override;
};

void TestGifWriterCallbackFailure::SetUp()
{
    TestGifWriter::SetUp();
    m_buffer.remaining = 0;
    m_output.exceptions(std::ios::badbit | std::ios::failbit);
    open_writer({2, 1});
}

TEST_F(TestGifWriterCallbackFailure, containsStreamExceptionsAtTheGiflibCallbackBoundary)
{
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_writer->error());
    EXPECT_FALSE(m_writer->finish());
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

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, expected, width, height);
}

INSTANTIATE_TEST_SUITE_P(UnsignedBounds, TestGifWriterSize,
    Values(std::pair{32768, 1}, std::pair{65535, 1}, std::pair{1, 32768}, std::pair{1, 65535}));

struct InvalidGifScreen
{
    const char *name;
    GifWriter::Screen screen;
};

void PrintTo(const InvalidGifScreen &value, std::ostream *str)
{
    *str << value.name;
}

class TestGifWriterInvalidScreen : public TestGifWriter, public WithParamInterface<InvalidGifScreen>
{
};

TEST_P(TestGifWriterInvalidScreen, rejectsInvalidScreenBeforeOpeningOutput)
{
    EXPECT_THROW((GifWriter{m_path, GetParam().screen, make_palette(2)}), std::logic_error);

    EXPECT_FALSE(std::filesystem::exists(m_path));
}

INSTANTIATE_TEST_SUITE_P(ScreenInputs, TestGifWriterInvalidScreen,
    Values(InvalidGifScreen{"ZeroWidth", {0, 1}}, InvalidGifScreen{"NegativeWidth", {-1, 1}},
        InvalidGifScreen{"ZeroHeight", {1, 0}}, InvalidGifScreen{"NegativeHeight", {1, -1}},
        InvalidGifScreen{"WidthOverflow", {65536, 1}}, InvalidGifScreen{"HeightOverflow", {1, 65536}},
        InvalidGifScreen{"ZeroResolution", {2, 1, 0}}, InvalidGifScreen{"ResolutionOverflow", {2, 1, 9}},
        InvalidGifScreen{"NegativeBackground", {2, 1, 8, -1}}, InvalidGifScreen{"BackgroundOverflow", {2, 1, 8, 2}}),
    [](const TestParamInfo<InvalidGifScreen> &info) { return info.param.name; });

class TestGifWriterInvalidPalette : public TestGifWriter, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterInvalidPalette, rejectsInvalidPaletteBeforeOpeningOutput)
{
    EXPECT_THROW((GifWriter{m_path, {2, 1}, make_palette(GetParam())}), std::logic_error);

    EXPECT_FALSE(std::filesystem::exists(m_path));
}

INSTANTIATE_TEST_SUITE_P(PaletteSizes, TestGifWriterInvalidPalette, Values(0, 1, 3, 257),
    [](const TestParamInfo<int> &info) { return "Colors" + std::to_string(info.param); });

} // namespace id::test
