// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/load_config.h>

#include <engine/pixel_limits.h>
#include <test_config_data.h>

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::test::config;
using namespace testing;

namespace id::test
{

TEST(TestVideoConfigReader, pixelLimits)
{
    EXPECT_EQ(65535, GIF_MAX_PIXELS);
    EXPECT_EQ(32767, MAX_PIXELS);
    EXPECT_EQ(2048, OLD_MAX_PIXELS);
    EXPECT_EQ(10, MIN_PIXELS);
}

TEST(TestVideoConfigReader, readsConfiguredRecordsAsText)
{
    std::ifstream input{ID_TEST_HOME_CONFIG_FILE};
    ASSERT_TRUE(input);
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(ID_TEST_GDI_FN, entry.key_name);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, entry.x_dots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, entry.y_dots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, entry.colors);
    EXPECT_EQ(ID_TEST_GDI_DRIVER, entry.driver_name);
    EXPECT_EQ(ID_TEST_GDI_COMMENT, entry.comment);
    const int first_line{entry.line_num};
    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(ID_TEST_DISK_FN, entry.key_name);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, entry.x_dots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, entry.y_dots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, entry.colors);
    EXPECT_EQ(ID_TEST_DISK_DRIVER, entry.driver_name);
    EXPECT_EQ(ID_TEST_DISK_COMMENT, entry.comment);
    EXPECT_EQ(first_line + 1, entry.line_num);
    EXPECT_FALSE(reader.next(entry));
    EXPECT_FALSE(reader.failed());
}

TEST(TestVideoConfigReader, parsesFinalLineWithoutTrailingNewline)
{
    std::istringstream input{"F1,320,200,256,disk,last char"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ("last char", entry.comment);
    EXPECT_FALSE(reader.next(entry));
    EXPECT_FALSE(reader.failed());
}

TEST(TestVideoConfigReader, preservesTextAndNormalizesControlCharacters)
{
    std::istringstream input{"; comment\r\nSF8\t,320,200,16, disk ,hello\tthere, again\r\n"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ("SF8 ", entry.key_name);
    EXPECT_EQ(" disk ", entry.driver_name);
    EXPECT_EQ("hello there, again", entry.comment);
    EXPECT_EQ(2, entry.line_num);
    EXPECT_FALSE(reader.next(entry));
    EXPECT_FALSE(reader.failed());
}

TEST(TestVideoConfigReader, truncatesInputLinesAtTheLegacyLimit)
{
    const std::string prefix{"F1,320,200,256,disk,"};
    std::istringstream input{prefix + std::string(200, 'x') + '\n'};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(std::string(120 - prefix.size(), 'x'), entry.comment);
}

TEST(TestVideoConfigReader, retainsLegacyNumericPrefixParsing)
{
    std::istringstream input{"F1,320wide,200high,256colors,disk,test\n"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(320, entry.x_dots);
    EXPECT_EQ(200, entry.y_dots);
    EXPECT_EQ(256, entry.colors);
}

TEST(TestVideoConfigReader, malformedInputStopsAfterTheAcceptedPrefix)
{
    std::istringstream input{"F1,320,200,256,disk,first\ninvalid\nF2,640,480,256,disk,last\n"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_FALSE(reader.next(entry));
    EXPECT_TRUE(reader.failed());
    EXPECT_FALSE(reader.next(entry));
    EXPECT_EQ("first", entry.comment);
    EXPECT_EQ(1, entry.line_num);
}

TEST(TestVideoConfigReader, commentsAndEndOfFileAreNotMalformed)
{
    std::istringstream input{"; first\n; last"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    EXPECT_FALSE(reader.next(entry));
    EXPECT_FALSE(reader.failed());
}

class TestVideoConfigGifDimension : public Test, public WithParamInterface<int>
{
};

TEST_P(TestVideoConfigGifDimension, acceptsModeUpToGifLimit)
{
    const int size{GetParam()};
    std::istringstream input{"F1," + std::to_string(size) + ',' + std::to_string(size) + ",256,disk,test\n"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(size, entry.x_dots);
    EXPECT_EQ(size, entry.y_dots);
    EXPECT_EQ(256, entry.colors);
    EXPECT_FALSE(reader.failed());
}

INSTANTIATE_TEST_SUITE_P(
    GifDimension, TestVideoConfigGifDimension, Values(MIN_PIXELS, MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

class TestVideoConfigMalformed : public Test, public WithParamInterface<const char *>
{
};

TEST_P(TestVideoConfigMalformed, rejectsMalformedRecord)
{
    std::istringstream input{GetParam()};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    EXPECT_FALSE(reader.next(entry));
    EXPECT_TRUE(reader.failed());
}

INSTANTIATE_TEST_SUITE_P(Records, TestVideoConfigMalformed,
    Values("\n", "F1,320,200,256,disk\n", "F1,9,200,256,disk,test\n", "F1,320,9,256,disk,test\n",
        "F1,65536,200,256,disk,test\n", "F1,320,65536,256,disk,test\n", "F1,320,200,3,disk,test\n"));

class TestVideoConfigColors : public Test, public WithParamInterface<int>
{
};

TEST_P(TestVideoConfigColors, acceptsLegacyColorCounts)
{
    std::istringstream input{"F1,320,200," + std::to_string(GetParam()) + ",disk,test\n"};
    VideoConfigReader reader{input};
    VideoConfigEntry entry;

    ASSERT_TRUE(reader.next(entry));
    EXPECT_EQ(GetParam(), entry.colors);
}

INSTANTIATE_TEST_SUITE_P(Counts, TestVideoConfigColors, Values(0, 2, 4, 16, 256));

} // namespace id::test
