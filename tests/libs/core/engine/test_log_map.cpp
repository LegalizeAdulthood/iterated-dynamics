// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/log_map.h>

#include <engine/calcfrac.h>
#include <engine/VideoInfo.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <ostream>
#include <vector>

using namespace id::engine;
using namespace id::misc;

namespace id::test
{

struct TestSetupLogTable : testing::Test
{
    ~TestSetupLogTable() override = default;

protected:
    void SetUp() override;

    ValueSaver<long> saved_max_iterations{g_max_iterations, 0};
    ValueSaver<long> saved_log_map_flag{g_log_map_flag, 0};
    ValueSaver<bool> saved_log_map_calculate{g_log_map_calculate, false};
    ValueSaver<long> saved_log_map_table_max_size{g_log_map_table_max_size, 0};
    ValueSaver<Version> saved_version{g_version, Version{1, 0, 0, 0, false}};
    ValueSaver<int> saved_colors{g_colors, 256};
    ValueSaver<std::vector<Byte>> saved_table{g_log_map_table};
};

void TestSetupLogTable::SetUp()
{
    Test::SetUp();
    g_log_map_table.clear();
}

static std::vector<int> linear_palette(const int first, const int last)
{
    std::vector<int> expected(257);
    std::iota(expected.begin(), expected.end(), 0);
    expected.front() = first;
    expected.back() = last;
    return expected;
}

static std::vector<int> prefix_palette(const std::size_t prefix_size)
{
    std::vector<int> expected(257, 1);
    std::iota(expected.begin() + prefix_size, expected.end(), 1);
    return expected;
}

static std::vector<int> old_palette()
{
    constexpr std::array<int, 257> expected{
        1, 1, 32, 51, 64, 75, 83, 90, 96, 102, 106, 111, 115, 118, 122, 125,            // 0
        128, 131, 133, 136, 138, 141, 143, 145, 147, 149, 150, 152, 154, 155, 157, 158, // 16
        160, 161, 163, 164, 165, 167, 168, 169, 170, 171, 172, 173, 175, 176, 177, 178, // 32
        179, 179, 180, 181, 182, 183, 184, 185, 186, 186, 187, 188, 189, 190, 190, 191, // 48
        192, 192, 193, 194, 195, 195, 196, 197, 197, 198, 198, 199, 200, 200, 201, 201, // 64
        202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207, 208, 208, 209, 209, 210, // 80
        210, 211, 211, 212, 212, 213, 213, 214, 214, 215, 215, 215, 216, 216, 217, 217, // 96
        217, 218, 218, 219, 219, 219, 220, 220, 221, 221, 221, 222, 222, 223, 223, 223, // 112
        224, 224, 224, 225, 225, 225, 226, 226, 226, 227, 227, 227, 228, 228, 228, 229, // 128
        229, 229, 230, 230, 230, 231, 231, 231, 232, 232, 232, 232, 233, 233, 233, 234, // 144
        234, 234, 234, 235, 235, 235, 236, 236, 236, 236, 237, 237, 237, 237, 238, 238, // 160
        238, 239, 239, 239, 239, 240, 240, 240, 240, 241, 241, 241, 241, 242, 242, 242, // 176
        242, 243, 243, 243, 243, 243, 244, 244, 244, 244, 245, 245, 245, 245, 246, 246, // 192
        246, 246, 246, 247, 247, 247, 247, 247, 248, 248, 248, 248, 249, 249, 249, 249, // 208
        249, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251, 252, 252, 252, 252, 252, // 224
        253, 253, 253, 253, 253, 253, 254, 254, 254, 254, 254, 255, 255, 255, 255, 255, // 240
        0,                                                                              // 256
    };
    return {expected.begin(), expected.end()};
}

struct LogTableInput
{
    const char *name;
    Version version;
    long max_iterations;
    long flag;
    long maximum_size;
    std::vector<int> expected;
};

void PrintTo(const LogTableInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestSetupLogTableInputs : public TestSetupLogTable, public testing::WithParamInterface<LogTableInput>
{
protected:
    void SetUp() override;
};

void TestSetupLogTableInputs::SetUp()
{
    TestSetupLogTable::SetUp();
    const LogTableInput &input{GetParam()};
    g_version = input.version;
    g_max_iterations = input.max_iterations;
    g_log_map_flag = input.flag;
    g_log_map_table_max_size = input.maximum_size;
    if (input.maximum_size > 0)
    {
        g_log_map_table.resize(input.maximum_size + 1);
    }
}

TEST_P(TestSetupLogTableInputs, buildsExpectedPaletteTable)
{
    setup_log_table();

    const LogTableInput &input{GetParam()};
    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(input.maximum_size, g_log_map_table_max_size);
    ASSERT_EQ(input.expected.size(), g_log_map_table.size());
    for (std::size_t i = 0; i < input.expected.size(); ++i)
    {
        EXPECT_EQ(input.expected[i], g_log_map_table[i]) << "index " << i;
    }
    EXPECT_THAT(g_log_map_table, testing::ElementsAreArray(input.expected));
}

INSTANTIATE_TEST_SUITE_P(TableInputs, TestSetupLogTableInputs,
    testing::Values(LogTableInput{"ZeroMaximum", Version{1, 0, 0, 0, false}, 0, 0, 0, {}},
        LogTableInput{"ZeroFlag", Version{1, 0, 0, 0, false}, 256, 0, 256, linear_palette(0, 0)},
        LogTableInput{"SkipSixteen", Version{1, 0, 0, 0, false}, 256, 16, 256, prefix_palette(17)},
        LogTableInput{"StandardPalette", Version{1, 0, 0, 0, false}, 256, 1, 256, linear_palette(1, 0)},
        LogTableInput{"OldPalette", Version{1, 0, 0, 0, false}, 256, -1, 256, old_palette()},
        LogTableInput{"SquareRootTwo", Version{1, 0, 0, 0, false}, 256, -2, 256, prefix_palette(2)},
        LogTableInput{"SquareRootSixteen", Version{1, 0, 0, 0, false}, 256, -16, 256, prefix_palette(16)},
        LogTableInput{"SquareRootTwoHundred", Version{1, 0, 0, 0, false}, 256, -200, 256, prefix_palette(200)},
        LogTableInput{"Legacy1730", parse_legacy_version(1730), 256, 1, 256, linear_palette(0, 255)}),
    [](const testing::TestParamInfo<LogTableInput> &info) { return info.param.name; });

struct LogCalculationInput
{
    const char *name;
    Version version;
    long expected;
};

void PrintTo(const LogCalculationInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLogTableCalculation : public TestSetupLogTable, public testing::WithParamInterface<LogCalculationInput>
{
protected:
    void SetUp() override;
};

void TestLogTableCalculation::SetUp()
{
    TestSetupLogTable::SetUp();
    g_version = GetParam().version;
    g_log_map_calculate = true;
    g_log_map_flag = 16;
    g_log_map_table_max_size = 256L;
    setup_log_table();
}

TEST_P(TestLogTableCalculation, calculatesVersionSpecificCompressionOffset)
{
    EXPECT_EQ(GetParam().expected, log_table_calc(18));
}

INSTANTIATE_TEST_SUITE_P(CalculationInputs, TestLogTableCalculation,
    testing::Values(LogCalculationInput{"Before2002", parse_legacy_version(2001), 3},
        LogCalculationInput{"From2002", parse_legacy_version(2002), 2}),
    [](const testing::TestParamInfo<LogCalculationInput> &info) { return info.param.name; });

} // namespace id::test
