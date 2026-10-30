// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/pixel.h>

#include "MemoryDiskStorage.h"

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/Potential.h>
#include <engine/VideoInfo.h>
#include <io/disk_storage.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <ostream>
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
    void SetUp() override;
    void TearDown() override;

    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{6, 5, 2, 3, 5.0, 4.0}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 10};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 9};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
};

void TestPixel::SetUp()
{
    Test::SetUp();
    s_reads.clear();
    s_writes.clear();
    set_pixel_functions(write_pixel, read_pixel);
}

void TestPixel::TearDown()
{
    set_pixel_functions(write_pixel, read_pixel);
    s_reads.clear();
    s_writes.clear();
    Test::TearDown();
}

TEST_F(TestPixel, readsApplyOffsetsWithoutADriver)
{
    EXPECT_EQ(43, get_color(1, 1));

    ASSERT_EQ(1U, s_reads.size());
    EXPECT_EQ(3, s_reads[0].x);
    EXPECT_EQ(4, s_reads[0].y);
    EXPECT_TRUE(s_writes.empty());
}

TEST_F(TestPixel, writesApplyOffsetsAndMaskWithoutADriver)
{
    put_color_a(1, 1, 31);

    EXPECT_THAT(s_writes, ElementsAre(std::array<int, 3>{3, 4, 15}));
    EXPECT_TRUE(s_reads.empty());
}

struct PixelPositionInput
{
    const char *name;
    int x;
    int y;
};

void PrintTo(const PixelPositionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestPixelOffscreenRead : public TestPixel, public WithParamInterface<PixelPositionInput>
{
};

TEST_P(TestPixelOffscreenRead, doesNotAccessBackend)
{
    EXPECT_EQ(0, get_color(GetParam().x, GetParam().y));

    EXPECT_TRUE(s_reads.empty());
}

INSTANTIATE_TEST_SUITE_P(Coordinates, TestPixelOffscreenRead,
    Values(PixelPositionInput{"BeforeLeft", -3, 0}, PixelPositionInput{"AboveTop", 0, -4},
        PixelPositionInput{"AfterRight", 8, 0}, PixelPositionInput{"BelowBottom", 0, 6}),
    [](const TestParamInfo<PixelPositionInput> &info) { return info.param.name; });

TEST_F(TestPixel, writeSpansApplyOffsetsWithoutMaskingStoredColors)
{
    const std::array<Byte, 3> pixels{31, 160, 255};

    write_span(1, 1, 3, pixels.data());

    EXPECT_THAT(s_writes,
        ElementsAre(std::array<int, 3>{3, 4, 31}, std::array<int, 3>{4, 4, 160}, std::array<int, 3>{5, 4, 255}));
}

TEST_F(TestPixel, readSpansApplyOffsets)
{
    std::array<Byte, 3> result{};

    read_span(1, 1, 3, result.data());

    EXPECT_THAT(result, ElementsAre(43, 44, 45));
    EXPECT_EQ(3U, s_reads.size());
}

struct PixelSpanInput
{
    const char *name;
    int row;
    int start;
    int stop;
};

void PrintTo(const PixelSpanInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestPixelClippedReadSpan : public TestPixel, public WithParamInterface<PixelSpanInput>
{
};

TEST_P(TestPixelClippedReadSpan, doesNotAccessBackendOrChangePixels)
{
    std::array<Byte, 2> pixels{12, 34};

    read_span(GetParam().row, GetParam().start, GetParam().stop, pixels.data());

    EXPECT_TRUE(s_reads.empty());
    EXPECT_TRUE(s_writes.empty());
    EXPECT_THAT(pixels, ElementsAre(12, 34));
}

INSTANTIATE_TEST_SUITE_P(ClippingInputs, TestPixelClippedReadSpan,
    Values(PixelSpanInput{"RowOutside", 6, 0, 1}, PixelSpanInput{"ColumnOutside", 0, 8, 9}),
    [](const TestParamInfo<PixelSpanInput> &info) { return info.param.name; });

class TestPixelClippedWriteSpan : public TestPixel, public WithParamInterface<PixelSpanInput>
{
};

TEST_P(TestPixelClippedWriteSpan, doesNotAccessBackendOrChangePixels)
{
    std::array<Byte, 2> pixels{12, 34};

    write_span(GetParam().row, GetParam().start, GetParam().stop, pixels.data());

    EXPECT_TRUE(s_reads.empty());
    EXPECT_TRUE(s_writes.empty());
    EXPECT_THAT(pixels, ElementsAre(12, 34));
}

INSTANTIATE_TEST_SUITE_P(ClippingInputs, TestPixelClippedWriteSpan,
    Values(PixelSpanInput{"RowOutside", 7, 0, 1}, PixelSpanInput{"ColumnOutside", 0, 8, 9}),
    [](const TestParamInfo<PixelSpanInput> &info) { return info.param.name; });

struct PixelBackendInput
{
    const char *name;
    WritePixel writer;
    ReadPixel reader;
};

void PrintTo(const PixelBackendInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestPixelInvalidSelection : public TestPixel, public WithParamInterface<PixelBackendInput>
{
};

TEST_P(TestPixelInvalidSelection, preservesActiveBackend)
{
    EXPECT_THROW(set_pixel_functions(GetParam().writer, GetParam().reader), std::logic_error);

    EXPECT_EQ(43, get_color(1, 1));
    put_color_a(1, 1, 31);

    EXPECT_THAT(s_writes, ElementsAre(std::array<int, 3>{3, 4, 15}));
}

INSTANTIATE_TEST_SUITE_P(MissingFunctions, TestPixelInvalidSelection,
    Values(PixelBackendInput{"Writer", nullptr, read_pixel}, PixelBackendInput{"Reader", write_pixel, nullptr}),
    [](const TestParamInfo<PixelBackendInput> &info) { return info.param.name; });

class TestPixelInvalidWrite : public TestPixel, public WithParamInterface<PixelPositionInput>
{
};

TEST_P(TestPixelInvalidWrite, doesNotAccessBackend)
{
    EXPECT_THROW(put_color_a(GetParam().x, GetParam().y, 1), std::logic_error);

    EXPECT_TRUE(s_writes.empty());
}

INSTANTIATE_TEST_SUITE_P(Coordinates, TestPixelInvalidWrite,
    Values(PixelPositionInput{"BeforeLeft", -3, 0}, PixelPositionInput{"AboveTop", 0, -4},
        PixelPositionInput{"AfterRight", 9, 0}, PixelPositionInput{"BelowBottom", 0, 7}),
    [](const TestParamInfo<PixelPositionInput> &info) { return info.param.name; });

TEST_F(TestPixel, customWriterIsNotDiskOutput)
{
    EXPECT_FALSE(disk_pixel_output());
}

class TestDiskPixel : public TestPixel
{
protected:
    void SetUp() override;
};

void TestDiskPixel::SetUp()
{
    TestPixel::SetUp();
    set_pixel_functions(id::io::write_disk_pixel, id::io::read_disk_pixel);
}

TEST_F(TestDiskPixel, failedReplacementPreservesDiskClassification)
{
    EXPECT_THROW(set_pixel_functions(nullptr, read_pixel), std::logic_error);

    EXPECT_TRUE(disk_pixel_output());
}

TEST_F(TestDiskPixel, switchingToCustomWriterChangesClassification)
{
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
    init_memory_disk_storage();
    open_memory_disk_storage(10, 18, 256);
    g_potential.store_16bit = true;
}

void TestPotentialPixel::TearDown()
{
    id::io::end_disk();
    EXPECT_TRUE(exit_check().empty());
    TestPixel::TearDown();
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
