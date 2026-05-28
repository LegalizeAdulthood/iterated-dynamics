// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadmap.h>

#include "expected_map.h"
#include "test_data.h"

#include <engine/color_utils.h>
#include <engine/spindac.h>
#include <io/library.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
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

TEST(TestValidateLuts, loadMap)
{
    ValueSaver saved_map_name{g_map_name, std::string{ID_TEST_MAP_DIR} + "/foo.map"};
    add_read_library(ID_TEST_HOME_DIR);

    const bool result{validate_luts(ID_TEST_MAP_FILE)};

    EXPECT_FALSE(result);
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_EQ(g_expected_map[i][0], g_dac_box[i][0]) << "index " << i << " red";
        EXPECT_EQ(g_expected_map[i][1], g_dac_box[i][1]) << "index " << i << " green";
        EXPECT_EQ(g_expected_map[i][2], g_dac_box[i][2]) << "index " << i << " blue";
    }
    clear_read_library_path();
}

TEST(TestValidateLuts, oldResetExpandsQuantizedMap)
{
    TempMap map{"test-loadmap-legacy-quantized.map", "84 168 252"};
    ValueSaver saved_version{g_version, Version{1, 3, 0, 0, false}};

    const bool result{validate_luts(map.string())};

    EXPECT_FALSE(result);
    EXPECT_EQ(expand_8bit_color(84), g_dac_box[0][0]);
    EXPECT_EQ(expand_8bit_color(168), g_dac_box[0][1]);
    EXPECT_EQ(expand_8bit_color(252), g_dac_box[0][2]);
    EXPECT_EQ(expand_8bit_color(84), g_dac_box[255][0]);
    EXPECT_EQ(expand_8bit_color(168), g_dac_box[255][1]);
    EXPECT_EQ(expand_8bit_color(252), g_dac_box[255][2]);
}

TEST(TestValidateLuts, modernResetKeepsQuantizedMapExact)
{
    TempMap map{"test-loadmap-modern-quantized.map", "84 168 252"};
    ValueSaver saved_version{g_version, Version{1, 3, 1, 0, false}};

    const bool result{validate_luts(map.string())};

    EXPECT_FALSE(result);
    EXPECT_EQ(84, g_dac_box[0][0]);
    EXPECT_EQ(168, g_dac_box[0][1]);
    EXPECT_EQ(252, g_dac_box[0][2]);
    EXPECT_EQ(84, g_dac_box[255][0]);
    EXPECT_EQ(168, g_dac_box[255][1]);
    EXPECT_EQ(252, g_dac_box[255][2]);
}

TEST(TestValidateLuts, oldResetKeepsNonQuantizedMapExact)
{
    TempMap map{"test-loadmap-legacy-non-quantized.map", "83 168 252"};
    ValueSaver saved_version{g_version, Version{1, 3, 0, 0, false}};

    const bool result{validate_luts(map.string())};

    EXPECT_FALSE(result);
    EXPECT_EQ(83, g_dac_box[0][0]);
    EXPECT_EQ(168, g_dac_box[0][1]);
    EXPECT_EQ(252, g_dac_box[0][2]);
    EXPECT_EQ(83, g_dac_box[255][0]);
    EXPECT_EQ(168, g_dac_box[255][1]);
    EXPECT_EQ(252, g_dac_box[255][2]);
}

} // namespace id::test
