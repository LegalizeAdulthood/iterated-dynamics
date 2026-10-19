// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/pixel_limits.h>
#include <io/decoder.h>
#include <io/GifDecoder.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace testing;

namespace id::test
{

namespace
{

using File = std::unique_ptr<std::FILE, decltype(&std::fclose)>;
using LineHandler = std::function<int(Byte *, int)>;

struct Code
{
    int value;
    int bits;
};

LineHandler s_line_handler;

int dispatch_line(Byte *pixels, const int count)
{
    // A nested decode may replace the legacy output callback.
    const LineHandler handler{s_line_handler};
    return handler(pixels, count);
}

File make_file(const std::vector<Byte> &data)
{
    File file{std::tmpfile(), std::fclose};
    if (!file || (!data.empty() && std::fwrite(data.data(), 1, data.size(), file.get()) != data.size()))
    {
        throw std::runtime_error("Can't create GIF decoder test input");
    }
    std::rewind(file.get());
    return file;
}

std::vector<Byte> image_data(
    const std::initializer_list<Code> codes, const int block_size = 255, const Byte code_size = 2)
{
    std::vector<Byte> packed;
    int bit_offset{};
    for (const Code code : codes)
    {
        for (int bit = 0; bit < code.bits; ++bit, ++bit_offset)
        {
            if (bit_offset % 8 == 0)
            {
                packed.push_back(0);
            }
            packed.back() |= static_cast<Byte>(((code.value >> bit) & 1) << (bit_offset % 8));
        }
    }
    std::vector<Byte> result{code_size};
    for (std::size_t offset = 0; offset < packed.size();)
    {
        const std::size_t count{std::min(static_cast<std::size_t>(block_size), packed.size() - offset)};
        result.push_back(static_cast<Byte>(count));
        result.insert(result.end(), packed.data() + offset, packed.data() + offset + count);
        offset += count;
    }
    result.push_back(0);
    return result;
}

} // namespace

class TestGifDecoder : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<decltype(g_out_line)> m_saved_out_line{g_out_line, dispatch_line};
    std::vector<std::vector<Byte>> m_lines;
};

void TestGifDecoder::SetUp()
{
    s_line_handler = [this](Byte *pixels, int count)
    {
        m_lines.emplace_back(pixels, pixels + count);
        return 0;
    };
}

void TestGifDecoder::TearDown()
{
    s_line_handler = {};
}

TEST_F(TestGifDecoder, handlesRepeatedClearAndEndingCodesWithoutConsumingBlockTerminator)
{
    const File file{make_file(image_data({{4, 3}, {4, 3}, {0, 3}, {1, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 2};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 1}};
    EXPECT_EQ(expected, m_lines);
    EXPECT_EQ(0, decoder.decode());
    EXPECT_EQ(expected, m_lines);
    EXPECT_EQ(0, std::getc(file.get()));
}

TEST_F(TestGifDecoder, endingImmediatelyAfterClearProducesNoLine)
{
    const File file{make_file(image_data({{4, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 2};

    EXPECT_EQ(0, decoder.decode());

    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestGifDecoder, missingInitialClearRetainsLegacyDecoding)
{
    const File file{make_file(image_data({{0, 3}, {1, 3}, {5, 4}}))};
    GifDecoder decoder{file.get(), 2};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 1}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, expandsDictionaryPrefixAndGrowsCodeWidth)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {1, 3}, {6, 3}, {5, 4}}))};
    GifDecoder decoder{file.get(), 4};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 1, 0, 1}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, expandsNextSlotCodesAcrossLines)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {6, 3}, {7, 3}, {5, 4}}))};
    GifDecoder decoder{file.get(), 2};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 0}, {0, 0}, {0, 0}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, codesCanCrossInputSubBlocks)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {1, 3}, {6, 3}, {5, 4}}, 1))};
    GifDecoder decoder{file.get(), 4};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 1, 0, 1}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, skipFactorsRetainOriginalRowAccounting)
{
    const File file{make_file(image_data(
        {{4, 3}, {0, 3}, {1, 3}, {4, 3}, {2, 3}, {3, 3}, {4, 3}, {1, 3}, {0, 3}, {4, 3}, {3, 3}, {2, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 2, 1, 1};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0}, {1}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, nestedInstancesKeepIndependentInputDictionaryStackAndLineBuffers)
{
    const File outer_file{make_file(image_data({{4, 3}, {0, 3}, {6, 3}, {7, 3}, {5, 4}}))};
    const File inner_file{make_file(image_data({{4, 3}, {2, 3}, {3, 3}, {5, 3}}))};
    GifDecoder outer{outer_file.get(), 2};
    GifDecoder inner{inner_file.get(), 1};
    bool nested{};
    std::vector<std::vector<Byte>> inner_lines;
    s_line_handler = [this, &inner, &inner_lines, &nested](Byte *pixels, int count)
    {
        const std::vector<Byte> outer_line{pixels, pixels + count};
        m_lines.push_back(outer_line);
        if (!nested)
        {
            nested = true;
            ValueSaver<LineHandler> saved_handler{s_line_handler, [&inner_lines](Byte *inner_pixels, int inner_count)
                {
                    inner_lines.emplace_back(inner_pixels, inner_pixels + inner_count);
                    return 0;
                }};
            EXPECT_EQ(0, inner.decode());
            EXPECT_EQ(outer_line, (std::vector<Byte>{pixels, pixels + count}));
        }
        return 0;
    };

    EXPECT_EQ(0, outer.decode());

    EXPECT_TRUE(nested);
    const std::vector<std::vector<Byte>> expected_outer{{0, 0}, {0, 0}, {0, 0}};
    const std::vector<std::vector<Byte>> expected_inner{{2}, {3}};
    EXPECT_EQ(expected_outer, m_lines);
    EXPECT_EQ(expected_inner, inner_lines);
    EXPECT_EQ(0, std::getc(outer_file.get()));
    EXPECT_EQ(0, std::getc(inner_file.get()));
}

TEST_F(TestGifDecoder, outOfRangeCodeAfterClearFallsBackToZero)
{
    const File file{make_file(image_data({{4, 3}, {7, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 1};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, outOfRangeDictionaryCodeUsesLegacyNextSlotFallback)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {7, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 3};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{0, 0, 0}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, endingDoesNotEmitLegacyPartialLine)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 2};

    EXPECT_EQ(0, decoder.decode());

    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestGifDecoder, rejectsInvalidWidthWithoutReadingInput)
{
    for (const int width : {0, -1, GIF_MAX_PIXELS + 1})
    {
        const File file{make_file(image_data({{4, 3}, {5, 3}}))};
        GifDecoder decoder{file.get(), width};

        EXPECT_EQ(-2, decoder.decode());
        EXPECT_EQ(2, std::getc(file.get()));
    }
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestGifDecoder, rejectsInvalidCodeSize)
{
    for (const int size : {1, 10, 255})
    {
        const File file{make_file({static_cast<Byte>(size)})};
        GifDecoder decoder{file.get(), 1};

        EXPECT_EQ(-20, decoder.decode());
        EXPECT_EQ(-20, decoder.decode());
    }
    EXPECT_TRUE(m_lines.empty());
}

TEST_F(TestGifDecoder, retainsNineBitMinimumCodeSizeCompatibility)
{
    const File file{make_file(image_data({{512, 10}, {255, 10}, {513, 10}}, 255, 9))};
    GifDecoder decoder{file.get(), 1};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{255}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, missingHeaderAndTruncatedDataDoNotContaminateNextInstance)
{
    {
        const File file{make_file({})};
        GifDecoder decoder{file.get(), 1};
        EXPECT_EQ(EOF, decoder.decode());
    }
    {
        const File file{make_file({2})};
        GifDecoder decoder{file.get(), 1};
        EXPECT_EQ(0, decoder.decode());
    }
    EXPECT_TRUE(m_lines.empty());
    const File file{make_file(image_data({{4, 3}, {2, 3}, {5, 3}}))};
    GifDecoder decoder{file.get(), 1};

    EXPECT_EQ(0, decoder.decode());

    const std::vector<std::vector<Byte>> expected{{2}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, callbackFailureStopsDecodingAndPreservesErrorResult)
{
    const File file{make_file(image_data({{4, 3}, {0, 3}, {1, 3}, {5, 3}}))};
    {
        ValueSaver<LineHandler> saved_handler{s_line_handler, [](Byte *, int) { return -73; }};
        GifDecoder decoder{file.get(), 1};
        EXPECT_EQ(-73, decoder.decode());
        EXPECT_EQ(-73, decoder.decode());
    }
    const File next_file{make_file(image_data({{4, 3}, {2, 3}, {5, 3}}))};
    GifDecoder next{next_file.get(), 1};

    EXPECT_EQ(0, next.decode());

    const std::vector<std::vector<Byte>> expected{{2}};
    EXPECT_EQ(expected, m_lines);
}

TEST_F(TestGifDecoder, rejectsMissingFileAndInvalidSkipConfiguration)
{
    EXPECT_THROW((GifDecoder{nullptr, 1}), std::logic_error);
    const File file{make_file(image_data({{4, 3}, {5, 3}}))};
    EXPECT_THROW((GifDecoder{file.get(), 1, -1, 0}), std::logic_error);
    EXPECT_THROW((GifDecoder{file.get(), 1, 0, -1}), std::logic_error);
}

} // namespace id::test
