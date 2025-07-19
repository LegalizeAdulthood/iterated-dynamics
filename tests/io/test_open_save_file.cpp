// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/save_file.h>

#include "test_data.h"

#include <io/special_dirs.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string_view>

using namespace testing;
namespace fs = std::filesystem;

using namespace id::test::data;

TEST(TestSaveDir, initialValue)
{
    EXPECT_EQ(get_documents_dir(), g_save_dir);
}

TEST(TestOpenSaveFile, filename)
{
    const std::string name{"tmp.txt"};
    if (fs::exists(name))
    {
        fs::remove(name);
    }
    const auto path{fs::path{ID_TEST_DATA_DIR} / name};
    if (fs::exists(path))
    {
        fs::remove(path);
    }
    ValueSaver save_dir{g_save_dir, ID_TEST_DATA_DIR};

    std::FILE *file{open_save_file(name, "w")};

    ASSERT_NE(nullptr, file);
    std::fclose(file);
    ASSERT_FALSE(fs::exists(name));
    ASSERT_TRUE(fs::exists(path));
    fs::remove(path);
}

TEST(TestOpenSaveFile, relativeDir)
{
    constexpr std::string_view name{"tmp.txt"};
    if (fs::exists(name))
    {
        fs::remove(name);
    }
    const auto relative{fs::path{ID_TEST_DATA_SUBDIR_NAME} / name};
    const auto path{fs::path{ID_TEST_DATA_DIR} / relative};
    if (fs::exists(path))
    {
        fs::remove(path);
    }
    ValueSaver save_dir{g_save_dir, ID_TEST_DATA_DIR};

    std::FILE *file{open_save_file(relative.string(), "w")};

    ASSERT_NE(nullptr, file);
    std::fclose(file);
    ASSERT_FALSE(fs::exists(name));
    ASSERT_TRUE(fs::exists(path));
    fs::remove(path);
}
