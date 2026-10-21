// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/pixel.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/Potential.h>
#include <engine/VideoInfo.h>
#include <io/disk_storage.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{

namespace
{

std::vector<Point2i> s_reads;
std::vector<std::array<int, 3>> s_writes;

int read_pixel(const int x, const int y)
{
    s_reads.push_back({x, y});
    return 10 * y + x;
}

void write_pixel(const int x, const int y, const int color)
{
    s_writes.push_back({x, y, color});
}

} // namespace

class TestPixel : public Test
{
protected:
    void SetUp() override
    {
        s_reads.clear();
        s_writes.clear();
        set_pixel_functions(write_pixel, read_pixel);
    }

    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{6, 5, 2, 3, 5.0, 4.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 10};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 9};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
};

TEST_F(TestPixel, pixelsApplyOffsetsAndMaskWithoutADriver)
{
    EXPECT_EQ(43, get_color(1, 1));
    put_color_a(1, 1, 31);

    ASSERT_EQ(1U, s_reads.size());
    EXPECT_EQ(3, s_reads[0].x);
    EXPECT_EQ(4, s_reads[0].y);
    EXPECT_THAT(s_writes, ElementsAre(std::array<int, 3>{3, 4, 15}));
}

TEST_F(TestPixel, offscreenReadsDoNotAccessTheBackend)
{
    EXPECT_EQ(0, get_color(-3, 0));
    EXPECT_EQ(0, get_color(0, -4));
    EXPECT_EQ(0, get_color(8, 0));
    EXPECT_EQ(0, get_color(0, 6));

    EXPECT_TRUE(s_reads.empty());
}

TEST_F(TestPixel, spansApplyOffsetsWithoutMaskingStoredColors)
{
    const std::array<Byte, 3> pixels{31, 160, 255};
    std::array<Byte, 3> result{};

    write_span(1, 1, 3, pixels.data());
    read_span(1, 1, 3, result.data());

    EXPECT_THAT(s_writes,
        ElementsAre(std::array<int, 3>{3, 4, 31}, std::array<int, 3>{4, 4, 160}, std::array<int, 3>{5, 4, 255}));
    EXPECT_THAT(result, ElementsAre(43, 44, 45));
    EXPECT_EQ(3U, s_reads.size());
}

TEST_F(TestPixel, clippedSpansDoNotAccessTheBackendOrChangePixels)
{
    std::array<Byte, 2> pixels{12, 34};

    read_span(6, 0, 1, pixels.data());
    read_span(0, 8, 9, pixels.data());
    write_span(7, 0, 1, pixels.data());
    write_span(0, 8, 9, pixels.data());

    EXPECT_TRUE(s_reads.empty());
    EXPECT_TRUE(s_writes.empty());
    EXPECT_THAT(pixels, ElementsAre(12, 34));
}

TEST_F(TestPixel, invalidSelectionPreservesTheActiveBackend)
{
    EXPECT_THROW(set_pixel_functions(nullptr, read_pixel), std::logic_error);
    EXPECT_THROW(set_pixel_functions(write_pixel, nullptr), std::logic_error);

    EXPECT_EQ(43, get_color(1, 1));
    put_color_a(1, 1, 31);

    EXPECT_THAT(s_writes, ElementsAre(std::array<int, 3>{3, 4, 15}));
}

TEST_F(TestPixel, invalidPixelWritesDoNotAccessTheBackend)
{
    EXPECT_THROW(put_color_a(-3, 0, 1), std::logic_error);
    EXPECT_THROW(put_color_a(0, -4, 1), std::logic_error);
    EXPECT_THROW(put_color_a(9, 0, 1), std::logic_error);
    EXPECT_THROW(put_color_a(0, 7, 1), std::logic_error);

    EXPECT_TRUE(s_writes.empty());
}

TEST_F(TestPixel, diskOutputDependsOnTheSelectedPixelWriter)
{
    EXPECT_FALSE(disk_pixel_output());

    set_pixel_functions(id::io::write_disk_pixel, id::io::read_disk_pixel);

    EXPECT_TRUE(disk_pixel_output());
    EXPECT_THROW(set_pixel_functions(nullptr, read_pixel), std::logic_error);
    EXPECT_TRUE(disk_pixel_output());
    set_pixel_functions(write_pixel, read_pixel);
    EXPECT_FALSE(disk_pixel_output());
}

class TestPotentialPixel : public TestPixel
{
protected:
    void SetUp() override;
    void TearDown() override;

    ValueSaver<Potential> m_saved_potential{g_potential};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 150};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 7};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_col{g_col, 1};
    ValueSaver<int> m_saved_row{g_row, 1};
};

void TestPotentialPixel::SetUp()
{
    TestPixel::SetUp();
    id::io::end_disk();
    init_memory();
    id::io::start_disk_storage(10, 18, 256, memory_alloc(id::io::DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    g_potential.store_16bit = true;
}

void TestPotentialPixel::TearDown()
{
    id::io::end_disk();
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestPotentialPixel, separatePixelOutputStoresBothPotentialBytesWithoutADriver)
{
    id::io::write_disk_pixel(3, 13, 99);

    EXPECT_EQ(7, potential(4.0, 150));

    EXPECT_EQ(7, id::io::read_disk_pixel(3, 4));
    EXPECT_EQ(0, id::io::read_disk_pixel(3, 13));
    EXPECT_TRUE(s_writes.empty());
}

TEST_F(TestPotentialPixel, diskPixelOutputLeavesHighByteForPlotting)
{
    set_pixel_functions(id::io::write_disk_pixel, id::io::read_disk_pixel);
    id::io::write_disk_pixel(3, 4, 99);
    id::io::write_disk_pixel(3, 13, 99);

    const int color{potential(4.0, 150)};

    EXPECT_EQ(7, color);
    EXPECT_EQ(99, id::io::read_disk_pixel(3, 4));
    EXPECT_EQ(0, id::io::read_disk_pixel(3, 13));
    put_color_a(1, 1, color);
    EXPECT_EQ(7, id::io::read_disk_pixel(3, 4));
}

} // namespace id::test
