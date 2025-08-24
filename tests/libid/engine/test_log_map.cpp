// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/id_data.h"

#include <engine/log_map.h>

#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>

namespace
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
};

void TestSetupLogTable::SetUp()
{
    g_log_map_table.clear();
}

}

TEST_F(TestSetupLogTable, zeroLogMapTableMaxSize)
{
    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
}

TEST_F(TestSetupLogTable, maxIterationsZeroLogMapFlag)
{
    g_max_iterations = 256;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    for (size_t i = 0; i < g_log_map_table.size(); ++i)
    {
        EXPECT_EQ(i % 256U, g_log_map_table[i]) << "index " << i;
    }
}

TEST_F(TestSetupLogTable, maxIterationsPositiveLogMapFlag)
{
    g_max_iterations = 256;
    constexpr int SKIP_COUNT{16};
    g_log_map_flag = SKIP_COUNT;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    for (long i = 0; i <= g_log_map_flag; ++i)
    {
        EXPECT_EQ(1, g_log_map_table[i]) << "index " << i;
    }
    for (size_t i = 1; i < g_log_map_table.size() - g_log_map_flag; ++i)
    {
        EXPECT_EQ(i, g_log_map_table[i + g_log_map_flag]) << "index " << i;
    }
}

TEST_F(TestSetupLogTable, standardPaletteLogMapFlag)
{
    g_max_iterations = 256;
    g_log_map_flag = 1;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    for (long i = 0; i <= g_log_map_flag; ++i)
    {
        EXPECT_EQ(1, g_log_map_table[i]) << "index " << i;
    }
    for (size_t i = 1; i < g_log_map_table.size() - g_log_map_flag; ++i)
    {
        EXPECT_EQ((i + 1) % 256U, g_log_map_table[i + g_log_map_flag]) << "index " << i;
    }
}

TEST_F(TestSetupLogTable, oldPaletteLogMapFlag)
{
    g_max_iterations = 256;
    g_log_map_flag = -1;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    std::array<int, 257> expected{
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
    EXPECT_THAT(g_log_map_table, testing::ElementsAreArray(expected));
}

TEST_F(TestSetupLogTable, sqrtPaletteLogMapFlagMinusTwo)
{
    g_max_iterations = 256;
    g_log_map_flag = -2;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    std::array<int, 257> expected{1, 1};
    std::iota(expected.begin() + 2, expected.end(), 1);
    EXPECT_EQ(expected.size(), g_log_map_table.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], g_log_map_table[i]) << "index " << i;
    }
    EXPECT_THAT(g_log_map_table, testing::ElementsAreArray(expected));
}

TEST_F(TestSetupLogTable, sqrtPaletteLogMapFlagMinusSixteen)
{
    g_max_iterations = 256;
    constexpr int COUNT{16};
    g_log_map_flag = -COUNT;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    std::array<int, 257> expected{};
    std::fill_n(expected.begin(), COUNT, 1);
    std::iota(expected.begin() + COUNT, expected.end(), 1);
    EXPECT_EQ(expected.size(), g_log_map_table.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], g_log_map_table[i]) << "index " << i;
    }
    EXPECT_THAT(g_log_map_table, testing::ElementsAreArray(expected));
}

TEST_F(TestSetupLogTable, sqrtPaletteLogMapFlagMinusTwoHundred)
{
    g_max_iterations = 256;
    constexpr int COUNT{200};
    g_log_map_flag = -COUNT;
    g_log_map_table_max_size = 256L;
    g_log_map_table.resize(g_log_map_table_max_size + 1);

    setup_log_table();

    EXPECT_FALSE(g_log_map_calculate);
    EXPECT_EQ(256U + 1U, g_log_map_table.size());
    EXPECT_EQ(256L, g_log_map_table_max_size);
    std::array<int, 257> expected{};
    std::fill_n(expected.begin(), COUNT, 1);
    std::iota(expected.begin() + COUNT, expected.end(), 1);
    EXPECT_EQ(expected.size(), g_log_map_table.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i], g_log_map_table[i]) << "index " << i;
    }
    EXPECT_THAT(g_log_map_table, testing::ElementsAreArray(expected));
}
