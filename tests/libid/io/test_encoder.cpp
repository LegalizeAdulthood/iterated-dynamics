// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/encoder.h>

#include <engine/pixel_limits.h>
#include <io/gif_extensions.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::test
{

TEST(TestEncoder, saveInfoPreservesActiveIdCompatibilityVersion)
{
    ValueSaver saved_version{g_version, Version{1, 3, 2, 7, false}};
    FractalInfo info{};

    setup_save_info(&info);

    EXPECT_EQ(2004, info.release);
    EXPECT_EQ(1, info.version_major);
    EXPECT_EQ(3, info.version_minor);
    EXPECT_EQ(2, info.version_patch);
    EXPECT_EQ(7, info.version_tweak);
    EXPECT_EQ((Version{1, 3, 2, 7, false}), fractal_info_version(info));
}

TEST(TestEncoder, saveInfoPreservesActiveLegacyCompatibilityVersion)
{
    ValueSaver saved_version{g_version, parse_legacy_version(1730)};
    FractalInfo info{};

    setup_save_info(&info);

    EXPECT_EQ(1730, info.release);
    EXPECT_EQ(0, info.version_major);
    EXPECT_EQ(0, info.version_minor);
    EXPECT_EQ(0, info.version_patch);
    EXPECT_EQ(0, info.version_tweak);
    EXPECT_EQ(parse_legacy_version(1730), fractal_info_version(info));
}

class TestGifUint16 : public testing::TestWithParam<int>
{
};

TEST_P(TestGifUint16, writesLittleEndianBytes)
{
    const int value{GetParam()};
    std::FILE *file{std::tmpfile()};
    ASSERT_NE(nullptr, file);

    ASSERT_TRUE(write_gif_uint16(file, value));
    std::rewind(file);

    std::array<unsigned char, 2> bytes{};
    EXPECT_EQ(1, std::fread(bytes.data(), bytes.size(), 1, file));
    std::fclose(file);

    EXPECT_EQ(value & 0xFF, bytes[0]);
    EXPECT_EQ(value >> 8, bytes[1]);
}

INSTANTIATE_TEST_SUITE_P(GifDimension, TestGifUint16, testing::Values(0, 32767, 32768, GIF_MAX_PIXELS));

TEST(TestEncoder, writeGifUint16RejectsOutOfRangeDimensions)
{
    std::FILE *file{std::tmpfile()};
    ASSERT_NE(nullptr, file);

    EXPECT_FALSE(write_gif_uint16(file, -1));
    EXPECT_FALSE(write_gif_uint16(file, GIF_MAX_PIXELS + 1));
    EXPECT_EQ(0, std::ftell(file));

    std::fclose(file);
}

} // namespace id::test
