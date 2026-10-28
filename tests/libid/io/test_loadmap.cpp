// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/loadmap.h>

#include "expected_map.h"
#include "ColorMapSaver.h"
#include "test_data.h"

#include <engine/color_utils.h>
#include <engine/color_state.h>
#include <engine/spindac.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <misc/Driver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::test::data;

namespace id::test
{

class TempMap
{
public:
    TempMap(const std::string &name, const std::string &line) :
        m_path(fs::current_path() / name)
    {
        std::ofstream out{m_path};
        for (int i = 0; i < 256; ++i)
        {
            out << line << '\n';
        }
    }

    ~TempMap()
    {
        std::error_code err;
        fs::remove(m_path, err);
    }

    std::string string() const
    {
        return m_path.string();
    }

private:
    fs::path m_path;
};

class TestValidateLuts : public testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    Byte m_default_palette[256][3]{};

private:
    ColorMapSaver m_palette;
    ValueSaver<Driver *> m_driver{g_driver, nullptr};
    ValueSaver<Version> m_version{g_version};
    ValueSaver<ColorState> m_state{g_color_state};
    ValueSaver<std::string> m_name{g_last_map_name};
    ValueSaver<std::string> m_map_name{g_map_name};
    ValueSaver<bool> m_specified{g_map_specified};
};

void TestValidateLuts::SetUp()
{
    Test::SetUp();
    clear_read_library_path();
    std::memcpy(m_default_palette, g_map_clut, sizeof(m_default_palette));
}

void TestValidateLuts::TearDown()
{
    std::memcpy(g_map_clut, m_default_palette, sizeof(m_default_palette));
    clear_read_library_path();
    Test::TearDown();
}

TEST_F(TestValidateLuts, loadMap)
{
    g_map_name = std::string{ID_TEST_MAP_DIR} + "/foo.map";
    add_read_library(ID_TEST_HOME_DIR);

    const PaletteLoadResult result{validate_luts(ID_TEST_MAP_FILE)};

    EXPECT_FALSE(result.failed);
    EXPECT_TRUE(result.diagnostic.empty());
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_EQ(g_expected_map[i][0], g_dac_box[i][0]) << "index " << i << " red";
        EXPECT_EQ(g_expected_map[i][1], g_dac_box[i][1]) << "index " << i << " green";
        EXPECT_EQ(g_expected_map[i][2], g_dac_box[i][2]) << "index " << i << " blue";
    }
}

struct MapCompatibilityInput
{
    const char *name;
    const char *filename;
    Version version;
    const char *rgb;
    std::array<int, 3> expected;
};

void PrintTo(const MapCompatibilityInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestValidateLutsCompatibility : public TestValidateLuts, public testing::WithParamInterface<MapCompatibilityInput>
{
protected:
    void SetUp() override;
};

void TestValidateLutsCompatibility::SetUp()
{
    TestValidateLuts::SetUp();
    g_version = GetParam().version;
}

TEST_P(TestValidateLutsCompatibility, loadsPaletteUsingVersionCompatibility)
{
    const MapCompatibilityInput &input{GetParam()};
    TempMap map{input.filename, input.rgb};

    const PaletteLoadResult result{validate_luts(map.string())};

    EXPECT_FALSE(result.failed);
    EXPECT_TRUE(result.diagnostic.empty());
    for (int channel = 0; channel < 3; ++channel)
    {
        EXPECT_EQ(input.expected[channel], g_dac_box[0][channel]) << "channel " << channel;
        EXPECT_EQ(input.expected[channel], g_dac_box[255][channel]) << "channel " << channel;
    }
}

INSTANTIATE_TEST_SUITE_P(CompatibilityInputs, TestValidateLutsCompatibility,
    testing::Values(
        MapCompatibilityInput{"LegacyQuantized", "test-loadmap-legacy-quantized.map", Version{1, 3, 0, 0, false},
            "84 168 252", {expand_8bit_color(84), expand_8bit_color(168), expand_8bit_color(252)}},
        MapCompatibilityInput{"ModernQuantized", "test-loadmap-modern-quantized.map", Version{1, 3, 1, 0, false},
            "84 168 252", {84, 168, 252}},
        MapCompatibilityInput{"LegacyNonQuantized", "test-loadmap-legacy-non-quantized.map", Version{1, 3, 0, 0, false},
            "83 168 252", {83, 168, 252}}),
    [](const testing::TestParamInfo<MapCompatibilityInput> &info) { return info.param.name; });

TEST_F(TestValidateLuts, missingMapPreservesPaletteAndMetadataWithoutUI)
{
    g_color_state = ColorState::UNKNOWN_MAP;
    g_last_map_name = "previous.map";
    Byte palette[256][3];
    std::memcpy(palette, g_dac_box, sizeof(palette));

    const PaletteLoadResult result{validate_luts("missing-palette-diagnostics.map")};

    EXPECT_TRUE(result.failed);
    EXPECT_EQ("Could not load color map missing-palette-diagnostics.map", result.diagnostic);
    EXPECT_EQ(0, std::memcmp(palette, g_dac_box, sizeof(palette)));
    EXPECT_EQ(ColorState::UNKNOWN_MAP, g_color_state);
    EXPECT_EQ("previous.map", g_last_map_name);
}

TEST_F(TestValidateLuts, malformedMapLoadsValidPrefixAndFillsRemainderWithoutUI)
{
    g_version = Version{1, 3, 1, 0, false};
    TempMap map{"test-loadmap-malformed.map", "257 258 259\n10 20"};

    const PaletteLoadResult result{validate_luts(map.string())};

    EXPECT_FALSE(result.failed);
    EXPECT_EQ(
        "Malformed map entry on line 1, only 2 channels but expected 3.\nSkipping remaining lines.", result.diagnostic);
    EXPECT_EQ(1, g_dac_box[0][0]);
    EXPECT_EQ(2, g_dac_box[0][1]);
    EXPECT_EQ(3, g_dac_box[0][2]);
    for (int color = 1; color < 256; ++color)
    {
        for (const Byte channel : g_dac_box[color])
        {
            EXPECT_EQ(40, channel);
        }
    }
    EXPECT_EQ(ColorState::MAP_FILE, g_color_state);
    EXPECT_EQ(map.string(), g_last_map_name);
}

TEST_F(TestValidateLuts, missingDefaultMapPreservesStoredPaletteWithoutUI)
{
    g_map_specified = true;

    const PaletteLoadResult result{set_color_palette_name("missing-default-palette.map")};

    EXPECT_TRUE(result.failed);
    EXPECT_EQ(0, std::memcmp(m_default_palette, g_map_clut, sizeof(m_default_palette)));
    EXPECT_TRUE(g_map_specified);
}

TEST_F(TestValidateLuts, malformedDefaultMapRetainsWarningAndStoresPartialPalette)
{
    g_version = Version{1, 3, 1, 0, false};
    g_map_specified = false;
    TempMap map{"test-loadmap-default-malformed.map", "1 2 3\n4 5"};

    const PaletteLoadResult result{set_color_palette_name(map.string())};

    EXPECT_FALSE(result.failed);
    EXPECT_FALSE(result.diagnostic.empty());
    EXPECT_TRUE(g_map_specified);
    EXPECT_EQ(0, std::memcmp(g_dac_box, g_map_clut, sizeof(g_map_clut)));
}

} // namespace id::test
