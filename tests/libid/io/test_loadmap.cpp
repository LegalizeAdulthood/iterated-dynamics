// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/loadmap.h>

#include "expected_map.h"
#include "test_data.h"

#include <io/library.h>
#include <misc/ValueSaver.h>
#include <ui/rotate.h>

#include <gtest/gtest.h>

using namespace id::io;
using namespace id::misc;
using namespace id::test::data;
using namespace id::ui;

namespace id::test
{

TEST(TestValidateLuts, loadMap)
{
    ValueSaver saved_map_name{g_map_name, std::string{ID_TEST_MAP_DIR} + "/foo.map"};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};

    const bool result{validate_luts(ID_TEST_MAP_FILE)};

    EXPECT_FALSE(result);
    for (int i = 0; i < 256; ++i)
    {
        EXPECT_EQ(g_expected_map[i][0], g_dac_box[i][0]) << "index " << i << " red";
        EXPECT_EQ(g_expected_map[i][1], g_dac_box[i][1]) << "index " << i << " green";
        EXPECT_EQ(g_expected_map[i][2], g_dac_box[i][2]) << "index " << i << " blue";
    }
}

} // namespace id::test
