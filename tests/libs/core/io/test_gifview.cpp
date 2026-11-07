// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/decoder.h>
#include <io/loadfile.h>

#include <MockDriver.h>

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/LogicalScreen.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel_limits.h>
#include <engine/Potential.h>
#include <engine/sound.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/lorenz.h>
#include <io/gifview.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <array>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

struct GifPaletteDetectionInput
{
    const char *name;
    Version version;
    std::array<Byte, 3> color;
    int count;
    bool expected;
};

void PrintTo(const GifPaletteDetectionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLegacy13GifPaletteDetection : public TestWithParam<GifPaletteDetectionInput>
{
protected:
    void SetUp() override;

    Byte m_palette[256][3]{};
};

void TestLegacy13GifPaletteDetection::SetUp()
{
    Test::SetUp();
    std::copy(GetParam().color.begin(), GetParam().color.end(), m_palette[0]);
}

TEST_P(TestLegacy13GifPaletteDetection, detectsPaletteCompatibility)
{
    EXPECT_EQ(GetParam().expected, backwards_id1_3_palette_needed(GetParam().version, m_palette, GetParam().count));
}

INSTANTIATE_TEST_SUITE_P(DetectionInputs, TestLegacy13GifPaletteDetection,
    Values(GifPaletteDetectionInput{"BeforeRange", Version{1, 3, 0, 0, false}, {252, 168, 84}, 1, false},
        GifPaletteDetectionInput{"RangeStart", Version{1, 3, 1, 0, false}, {252, 168, 84}, 1, true},
        GifPaletteDetectionInput{"WithinRange", Version{1, 3, 2, 9, false}, {252, 168, 84}, 1, true},
        GifPaletteDetectionInput{"AfterRange", Version{1, 3, 3, 0, false}, {252, 168, 84}, 1, false},
        GifPaletteDetectionInput{"NonQuantized", Version{1, 3, 1, 0, false}, {253, 0, 0}, 1, false}),
    [](const TestParamInfo<GifPaletteDetectionInput> &info) { return info.param.name; });

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
