// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/load_config.h>

#include "MockDriver.h"
#include "test_config_data.h"

#include <engine/id_data.h>
#include <io/CurrentPathSaver.h>
#include <io/locate_input_file.h>
#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::test::config;
using namespace testing;

namespace id::test
{

TEST(TestLocateConfigFile, preferCurrentDirectory)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};
    CurrentPathSaver saved_current{ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_input_file(ID_TEST_CFG)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_CONFIG_FILE}, result);
}

TEST(TestLocateConfigFile, preferConfigFileFromSaveDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_input_file(ID_TEST_CFG)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_SAVE_CONFIG_FILE}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToSearchDir1)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_input_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToSearchDir2)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_input_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}

TEST(TestLocateConfigFile, loadToolsFileFallbackToCurrentDir)
{
    ValueSaver saved_save_dir{g_save_dir, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir1{g_fractal_search_dir1, ID_TEST_SAVE_DIR};
    ValueSaver saved_search_dir2{g_fractal_search_dir2, ID_TEST_SAVE_DIR};
    CurrentPathSaver saved_current_path{ID_TEST_HOME_DIR};

    const std::filesystem::path result{locate_input_file(ID_TEST_INI)};

    EXPECT_EQ(std::filesystem::path{ID_TEST_HOME_TOOLS_INI}, result);
}

} // namespace id::test
