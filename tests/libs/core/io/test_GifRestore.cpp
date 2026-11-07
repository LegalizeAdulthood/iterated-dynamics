// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/GifRestore.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ostream>
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

std::vector<Byte> gif_header()
{
    return {'G', 'I', 'F', '8', '7', 'a', 2, 0, 2, 0, 0x80, 0, 0, 255, 0, 0, 0, 0, 0};
}

void append_image(std::vector<Byte> &data, const Byte left = 0, const Byte top = 0, const bool local_palette = false)
{
    data.insert(data.end(), {',', left, 0, top, 0, 2, 0, 2, 0, static_cast<Byte>(local_palette ? 0x80 : 0)});
    if (local_palette)
    {
        data.insert(data.end(), {0, 0, 255, 255, 255, 0});
    }
    // Three-bit codes: clear, 0, 1, clear, 1, 0, end.
    data.insert(data.end(), {2, 3, 0x44, 0x18, 0x14, 0});
}

std::vector<Byte> gif_data()
{
    std::vector<Byte> data{gif_header()};
    append_image(data);
    data.push_back(';');
    return data;
}

std::vector<std::vector<Byte>> drain(GifRestore &restore)
{
    std::vector<std::vector<Byte>> lines;
    for (int steps = 0; !restore.done() && steps < 100; ++steps)
    {
        restore.iterate();
        if (restore.state() == GifRestore::State::LINE)
        {
            lines.push_back(restore.line());
        }
    }
    EXPECT_TRUE(restore.done());
    return lines;
}

} // namespace

class TestGifRestore : public Test
{
protected:
    std::filesystem::path write_file(const std::vector<Byte> &data);
    std::filesystem::path write_image(const std::vector<std::vector<Byte>> &rows);
    void TearDown() override;

    std::vector<std::filesystem::path> m_paths;
};

std::filesystem::path TestGifRestore::write_file(const std::vector<Byte> &data)
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{std::string{info->test_suite_name()} + "." + info->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    const std::filesystem::path path{std::filesystem::temp_directory_path() /
        ("id-GifRestore-" + name + "-" + std::to_string(m_paths.size()) + ".gif")};
    m_paths.push_back(path);
    std::ofstream output{path, std::ios::binary};
    if (!data.empty())
    {
        output.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    }
    output.close();
    if (!output)
    {
        throw std::runtime_error("Can't create GIF restore test input");
    }
    return path;
}

std::filesystem::path TestGifRestore::write_image(const std::vector<std::vector<Byte>> &rows)
{
    const std::filesystem::path path{write_file({})};
    int error{};
    std::unique_ptr<GifFileType, void (*)(GifFileType *)> output{EGifOpenFileName(path.string().c_str(), false, &error),
        [](GifFileType *gif)
        {
            int close_error{};
            EGifCloseFile(gif, &close_error);
        }};
    std::unique_ptr<ColorMapObject, decltype(&GifFreeMapObject)> palette{
        GifMakeMapObject(16, nullptr), GifFreeMapObject};
    if (!output || !palette)
    {
        throw std::runtime_error("Can't create GIF restore test image");
    }
    for (int i = 0; i < palette->ColorCount; ++i)
    {
        palette->Colors[i] = {static_cast<Byte>(i * 17), static_cast<Byte>(255 - i * 17), static_cast<Byte>(i)};
    }
    const int width{static_cast<int>(rows.front().size())};
    const int height{static_cast<int>(rows.size())};
    if (EGifPutScreenDesc(output.get(), width, height, 4, 0, palette.get()) == GIF_ERROR ||
        EGifPutImageDesc(output.get(), 0, 0, width, height, false, nullptr) == GIF_ERROR)
    {
        throw std::runtime_error("Can't write GIF restore test image header");
    }
    for (std::vector<Byte> row : rows)
    {
        if (EGifPutLine(output.get(), row.data(), width) == GIF_ERROR)
        {
            throw std::runtime_error("Can't write GIF restore test image row");
        }
    }
    if (EGifCloseFile(output.release(), &error) == GIF_ERROR)
    {
        throw std::runtime_error("Can't close GIF restore test image");
    }
    return path;
}

void TestGifRestore::TearDown()
{
    for (const std::filesystem::path &path : m_paths)
    {
        std::filesystem::remove(path);
    }
}

class TestGifRestoreInput : public TestGifRestore
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void TestGifRestoreInput::SetUp()
{
    m_path = write_file(gif_data());
}

TEST_F(TestGifRestoreInput, separatesHeaderImageLinesAndCompletion)
{
    GifRestore restore{m_path};
    EXPECT_FALSE(restore.done());
    EXPECT_EQ(GifRestore::State::BLOCK, restore.state());
    EXPECT_EQ(2, restore.width());
    EXPECT_EQ(2, restore.height());
    const std::vector<std::array<Byte, 3>> expected_palette{{255, 0, 0}, {0, 0, 0}};
    EXPECT_EQ(expected_palette, restore.palette());

    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE, restore.state());
    EXPECT_EQ(0, restore.image().left);
    EXPECT_EQ(0, restore.image().top);
    EXPECT_EQ(2, restore.image().width);
    EXPECT_EQ(2, restore.image().height);
    restore.iterate();
    EXPECT_EQ(GifRestore::State::LINE, restore.state());
    EXPECT_EQ((std::vector<Byte>{0, 1}), restore.line());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::LINE, restore.state());
    EXPECT_EQ((std::vector<Byte>{1, 0}), restore.line());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE_END, restore.state());
    EXPECT_FALSE(restore.done());
    restore.iterate();
    EXPECT_TRUE(restore.done());
    EXPECT_EQ(0, restore.result());
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

TEST_F(TestGifRestoreInput, iterationAfterCompletionPreservesSuccess)
{
    GifRestore restore{m_path};
    drain(restore);
    ASSERT_EQ(0, restore.result());

    restore.iterate();

    EXPECT_EQ(0, restore.result());
}

TEST_F(TestGifRestore, restoresScanlinesAcrossCompressedDataSubBlocks)
{
    std::vector<Byte> data{gif_data()};
    // Split the three-byte compressed block into single-byte sub-blocks.
    data.erase(data.begin() + 30, data.begin() + 34);
    data.insert(data.begin() + 30, {1, 0x44, 1, 0x18, 1, 0x14});
    GifRestore restore{write_file(data)};
    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}};

    EXPECT_EQ(expected, drain(restore));
    EXPECT_EQ(0, restore.result());
}

TEST_F(TestGifRestore, skipsExtensionSubBlocksBeforeDecodingImage)
{
    std::vector<Byte> data{gif_header()};
    data.insert(data.end(), {'!', 0xFE, 3, 'a', 'b', 'c', 2, 'd', 'e', 0});
    append_image(data);
    data.push_back(';');
    GifRestore restore{write_file(data)};

    restore.iterate();
    EXPECT_EQ(GifRestore::State::EXTENSION, restore.state());
    EXPECT_TRUE(restore.line().empty());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::BLOCK, restore.state());
    restore.iterate();
    EXPECT_EQ(GifRestore::State::IMAGE, restore.state());
    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}};
    EXPECT_EQ(expected, drain(restore));
    EXPECT_EQ(0, restore.result());
}

TEST_F(TestGifRestore, retainsImageBoundariesPositionsAndGlobalPalette)
{
    std::vector<Byte> data{gif_header()};
    data[6] = 5;
    data[8] = 6;
    append_image(data);
    data.insert(data.end(), {'!', 0xFE, 1, 'x', 0});
    append_image(data, 3, 4, true);
    data.push_back(';');
    GifRestore restore{write_file(data)};
    std::vector<GifRestore::Image> images;
    std::vector<std::vector<Byte>> lines;
    int endings{};

    for (int steps = 0; !restore.done() && steps < 100; ++steps)
    {
        restore.iterate();
        if (restore.state() == GifRestore::State::IMAGE)
        {
            images.push_back(restore.image());
        }
        else if (restore.state() == GifRestore::State::LINE)
        {
            lines.push_back(restore.line());
        }
        else if (restore.state() == GifRestore::State::IMAGE_END)
        {
            ++endings;
        }
    }
    EXPECT_TRUE(restore.done());
    EXPECT_EQ(0, restore.result());
    EXPECT_EQ(2, endings);
    ASSERT_EQ(2, images.size());
    EXPECT_EQ(5, restore.width());
    EXPECT_EQ(6, restore.height());
    EXPECT_EQ(0, images[0].left);
    EXPECT_EQ(0, images[0].top);
    EXPECT_EQ(2, images[0].width);
    EXPECT_EQ(2, images[0].height);
    EXPECT_EQ(3, images[1].left);
    EXPECT_EQ(4, images[1].top);
    EXPECT_EQ(2, images[1].width);
    EXPECT_EQ(2, images[1].height);
    EXPECT_EQ((std::array<Byte, 3>{255, 0, 0}), restore.palette()[0]);
    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}, {0, 1}, {1, 0}};
    EXPECT_EQ(expected, lines);
}

enum class SamplingImage
{
    MINIMAL,
    UNEQUAL_SKIPS,
    POTENTIAL
};

struct GifSamplingCase
{
    const char *name;
    SamplingImage image;
    int skip_x;
    int skip_y;
    std::vector<std::vector<Byte>> expected;
};

void PrintTo(const GifSamplingCase &value, std::ostream *str)
{
    *str << value.name;
}

class GifSamplingTest : public TestGifRestore, public WithParamInterface<GifSamplingCase>
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void GifSamplingTest::SetUp()
{
    switch (GetParam().image)
    {
    case SamplingImage::MINIMAL:
        m_path = write_file(gif_data());
        break;

    case SamplingImage::UNEQUAL_SKIPS:
        m_path = write_image({
            {0, 1, 2, 3, 4, 5, 6},
            {7, 8, 9, 10, 11, 12, 13},
            {14, 15, 0, 1, 2, 3, 4},
            {5, 6, 7, 8, 9, 10, 11},
            {12, 13, 14, 15, 0, 1, 2},
        });
        break;

    case SamplingImage::POTENTIAL:
        m_path = write_image({
            {0, 1, 2, 3, 8, 9, 10, 11},
            {4, 5, 6, 7, 12, 13, 14, 15},
            {3, 2, 1, 0, 11, 10, 9, 8},
        });
        break;
    }
}

class TestGifPixelSampling : public GifSamplingTest
{
};

TEST_P(TestGifPixelSampling, preservesSampledPixels)
{
    GifRestore restore{m_path, GetParam().skip_x, GetParam().skip_y};

    EXPECT_EQ(GetParam().expected, drain(restore));
    EXPECT_EQ(0, restore.result());
}

INSTANTIATE_TEST_SUITE_P(SamplingInputs, TestGifPixelSampling,
    Values(GifSamplingCase{"WiderSkip", SamplingImage::UNEQUAL_SKIPS, 2, 1, {{0, 3, 6}, {14, 1, 4}, {12, 15, 2}}},
        GifSamplingCase{"TallerSkip", SamplingImage::UNEQUAL_SKIPS, 1, 2, {{0, 2, 4, 6}, {5, 7, 9, 11}}},
        GifSamplingCase{"MinimalImage", SamplingImage::MINIMAL, 1, 1, {{0}}}),
    [](const TestParamInfo<GifSamplingCase> &info) { return info.param.name; });

class TestGifPotentialSampling : public GifSamplingTest
{
};

TEST_P(TestGifPotentialSampling, preservesSampledColorAndLowBytePairs)
{
    GifRestore restore{m_path, GetParam().skip_x, GetParam().skip_y, true};

    EXPECT_EQ(GetParam().expected, drain(restore));
    EXPECT_EQ(0, restore.result());
    if (GetParam().image == SamplingImage::MINIMAL)
    {
        EXPECT_EQ(2, restore.image().width);
    }
    else
    {
        EXPECT_EQ(8, restore.width());
        EXPECT_EQ(3, restore.height());
        EXPECT_EQ(8, restore.image().width);
        EXPECT_EQ(3, restore.image().height);
        ASSERT_EQ(16, restore.palette().size());
        EXPECT_EQ((std::array<Byte, 3>{0, 255, 0}), restore.palette().front());
        EXPECT_EQ((std::array<Byte, 3>{255, 0, 15}), restore.palette().back());
    }
}

INSTANTIATE_TEST_SUITE_P(SamplingInputs, TestGifPotentialSampling,
    Values(GifSamplingCase{"EveryPixel", SamplingImage::POTENTIAL, 0, 0,
               {{0, 1, 2, 3}, {8, 9, 10, 11}, {4, 5, 6, 7}, {12, 13, 14, 15}, {3, 2, 1, 0}, {11, 10, 9, 8}}},
        GifSamplingCase{"Subsampled", SamplingImage::POTENTIAL, 1, 1, {{0, 2}, {8, 10}, {3, 1}, {11, 9}}},
        GifSamplingCase{"MinimalImage", SamplingImage::MINIMAL, 0, 0, {{0}, {1}, {1}, {0}}}),
    [](const TestParamInfo<GifSamplingCase> &info) { return info.param.name; });

struct PotentialDescriptorCase
{
    const char *name;
    std::vector<std::pair<size_t, Byte>> mutations;
};

void PrintTo(const PotentialDescriptorCase &value, std::ostream *str)
{
    *str << value.name;
}

class TestPotentialGifDescriptor : public TestGifRestore, public WithParamInterface<PotentialDescriptorCase>
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void TestPotentialGifDescriptor::SetUp()
{
    std::vector<Byte> data{gif_data()};
    for (const std::pair<size_t, Byte> &mutation : GetParam().mutations)
    {
        data[mutation.first] = mutation.second;
    }
    m_path = write_file(data);
}

TEST_P(TestPotentialGifDescriptor, rejectsInconsistentDimensionsOrOffsets)
{
    GifRestore restore{m_path, 0, 0, true};

    restore.iterate();

    EXPECT_TRUE(restore.done());
    EXPECT_EQ(-1, restore.result());
    EXPECT_TRUE(restore.line().empty());
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

INSTANTIATE_TEST_SUITE_P(DescriptorInputs, TestPotentialGifDescriptor,
    Values(PotentialDescriptorCase{"NarrowerThan640ColumnScreen", {{6, 0x80}, {7, 0x02}}},
        PotentialDescriptorCase{"ScreenWidthMismatch", {{6, 4}}},
        PotentialDescriptorCase{"ScreenHeightMismatch", {{8, 4}}},
        PotentialDescriptorCase{"LeftOffsetMismatch", {{20, 2}}},
        PotentialDescriptorCase{"TopOffsetMismatch", {{22, 2}}},
        PotentialDescriptorCase{"ImageWidthMismatch", {{24, 4}}},
        PotentialDescriptorCase{"ImageHeightMismatch", {{26, 4}}},
        PotentialDescriptorCase{"OddMatchingWidths", {{6, 3}, {24, 3}}}),
    [](const TestParamInfo<PotentialDescriptorCase> &info) { return info.param.name; });

TEST_F(TestGifRestore, independentRestoresRetainInputAndLineBuffers)
{
    GifRestore first{write_file(gif_data())};
    GifRestore second{write_file(gif_data())};
    first.iterate();
    first.iterate();
    const std::vector<Byte> &line{first.line()};
    const Byte *pixels{line.data()};

    const std::vector<std::vector<Byte>> expected{{0, 1}, {1, 0}};
    EXPECT_EQ(expected, drain(second));
    EXPECT_EQ((std::vector<Byte>{0, 1}), line);
    EXPECT_EQ(pixels, line.data());
    first.iterate();
    EXPECT_EQ((std::vector<Byte>{1, 0}), first.line());
    EXPECT_TRUE(drain(first).empty());
    EXPECT_EQ(0, first.result());
}

TEST_F(TestGifRestore, closesFileWhenAbandonedBetweenLines)
{
    const std::filesystem::path path{write_file(gif_data())};
    {
        GifRestore restore{path};
        restore.iterate();
        restore.iterate();
        EXPECT_EQ(GifRestore::State::LINE, restore.state());
    }
    EXPECT_TRUE(std::filesystem::remove(path));
}

class TestTruncatedGifRestore : public TestGifRestore, public WithParamInterface<size_t>
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void TestTruncatedGifRestore::SetUp()
{
    std::vector<Byte> data{gif_data()};
    ASSERT_LT(GetParam(), data.size());
    data.resize(GetParam());
    m_path = write_file(data);
}

TEST_P(TestTruncatedGifRestore, reportsFailureAndClosesInput)
{
    GifRestore restore{m_path};

    drain(restore);

    EXPECT_LT(restore.result(), 0);
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

INSTANTIATE_TEST_SUITE_P(ByteCutoffs, TestTruncatedGifRestore, Range<size_t>(0, gif_data().size()),
    [](const TestParamInfo<size_t> &info) { return "Bytes" + std::to_string(info.param); });

struct GifCorruptionCase
{
    const char *name;
    size_t offset;
    Byte value;
    bool expect_no_output;
};

void PrintTo(const GifCorruptionCase &value, std::ostream *str)
{
    *str << value.name;
}

class TestCorruptGifRestore : public TestGifRestore, public WithParamInterface<GifCorruptionCase>
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void TestCorruptGifRestore::SetUp()
{
    std::vector<Byte> data{gif_data()};
    data[GetParam().offset] = GetParam().value;
    m_path = write_file(data);
}

TEST_P(TestCorruptGifRestore, reportsFailureAndClosesInput)
{
    GifRestore restore{m_path};

    const std::vector<std::vector<Byte>> lines{drain(restore)};

    EXPECT_LT(restore.result(), 0);
    if (GetParam().expect_no_output)
    {
        EXPECT_TRUE(lines.empty());
        EXPECT_EQ(-1, restore.result());
    }
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

// giflib checks the GIF signature, not the old decoder's version range.
INSTANTIATE_TEST_SUITE_P(CorruptInputs, TestCorruptGifRestore,
    Values(GifCorruptionCase{"Signature", 0, 0, false}, GifCorruptionCase{"PackedScreenFields", 10, 0, false},
        GifCorruptionCase{"ImageMarker", 19, 0, false}, GifCorruptionCase{"ImageWidth", 24, 0, false},
        GifCorruptionCase{"ZeroMinimumCodeSize", 29, 0, false}, GifCorruptionCase{"DataTerminator", 34, 1, false},
        // The custom decoder accepted 9; giflib requires at most 8.
        GifCorruptionCase{"NineBitMinimumCodeSize", 29, 9, true}),
    [](const TestParamInfo<GifCorruptionCase> &info) { return info.param.name; });

struct TruncatedGifComponent
{
    const char *name;
    std::vector<Byte> data;
};

void PrintTo(const TruncatedGifComponent &value, std::ostream *str)
{
    *str << value.name;
}

static std::vector<TruncatedGifComponent> truncated_components()
{
    TruncatedGifComponent extension{"Extension", gif_header()};
    extension.data.insert(extension.data.end(), {'!', 0xFE, 3, 'x'});
    TruncatedGifComponent palette{"LocalPalette", gif_header()};
    palette.data.insert(palette.data.end(), {',', 0, 0, 0, 0, 2, 0, 2, 0, 0x80, 255});
    return {extension, palette};
}

class TestTruncatedGifComponent : public TestGifRestore, public WithParamInterface<TruncatedGifComponent>
{
protected:
    void SetUp() override;

    std::filesystem::path m_path;
};

void TestTruncatedGifComponent::SetUp()
{
    m_path = write_file(GetParam().data);
}

TEST_P(TestTruncatedGifComponent, rejectsInputWithoutProducingOutput)
{
    GifRestore restore{m_path};

    EXPECT_TRUE(drain(restore).empty());

    EXPECT_EQ(-1, restore.result());
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

INSTANTIATE_TEST_SUITE_P(ComponentInputs, TestTruncatedGifComponent, ValuesIn(truncated_components()),
    [](const TestParamInfo<TruncatedGifComponent> &info) { return info.param.name; });

TEST_F(TestGifRestore, discardedRowsStillValidateCompressedDataTermination)
{
    std::vector<Byte> data{gif_data()};
    data.resize(data.size() - 2);
    GifRestore restore{write_file(data), 0, 10};

    EXPECT_EQ((std::vector<std::vector<Byte>>{{0, 1}}), drain(restore));
    EXPECT_EQ(-1, restore.result());
}

TEST_F(TestGifRestoreInput, missingFileReportsFailure)
{
    ASSERT_TRUE(std::filesystem::remove(m_path));

    GifRestore missing{m_path};

    EXPECT_TRUE(missing.done());
    EXPECT_EQ(-1, missing.result());
}

class TestGifRestoreNegativeSkips : public TestGifRestoreInput, public WithParamInterface<std::pair<int, int>>
{
};

TEST_P(TestGifRestoreNegativeSkips, rejectsNegativeSkipFactor)
{
    EXPECT_THROW((GifRestore{m_path, GetParam().first, GetParam().second}), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(SkipInputs, TestGifRestoreNegativeSkips,
    Values(std::pair<int, int>{-1, 0}, std::pair<int, int>{0, -1}),
    [](const TestParamInfo<std::pair<int, int>> &info) { return info.param.first < 0 ? "NegativeX" : "NegativeY"; });

} // namespace id::test
