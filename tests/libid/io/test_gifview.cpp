// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadfile.h>

#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

using namespace id::io;
using namespace id::misc;

namespace id::test
{

TEST(TestLegacy13GifPalette, detectsQuantizedPaletteInVersionRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 252;
    palette[0][1] = 168;
    palette[0][2] = 84;

    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 0, 0, false}, palette, 1));
    EXPECT_TRUE(backwards_id1_3_palette_needed(Version{1, 3, 1, 0, false}, palette, 1));
    EXPECT_TRUE(backwards_id1_3_palette_needed(Version{1, 3, 2, 9, false}, palette, 1));
    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 3, 0, false}, palette, 1));
}

TEST(TestLegacy13GifPalette, rejectsNonQuantizedPaletteInVersionRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 253;

    EXPECT_FALSE(backwards_id1_3_palette_needed(Version{1, 3, 1, 0, false}, palette, 1));
}

TEST(TestLegacy13GifPalette, expandsQuantizedPaletteToFullRange)
{
    Byte palette[256][3]{};
    palette[0][0] = 0;
    palette[0][1] = 4;
    palette[0][2] = 84;
    palette[1][0] = 168;
    palette[1][1] = 252;
    palette[1][2] = 128;

    ValueSaver saved_file_version{g_file_version, Version{1, 3, 1, 0, false}};

    backwards_id1_3_palette(palette, 2);

    EXPECT_EQ(0, palette[0][0]);
    EXPECT_EQ(4, palette[0][1]);
    EXPECT_EQ(85, palette[0][2]);
    EXPECT_EQ(170, palette[1][0]);
    EXPECT_EQ(255, palette[1][1]);
    EXPECT_EQ(130, palette[1][2]);
}

} // namespace id::test
