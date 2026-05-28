// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/encoder.h>

#include <io/gif_extensions.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

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

} // namespace id::test
