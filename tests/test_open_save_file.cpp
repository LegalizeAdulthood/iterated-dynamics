#include <save_file.h>

#include "test_data.h"

#include <special_dirs.h>
#include <value_saver.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <string_view>

using namespace testing;
namespace fs = std::filesystem;

TEST(TestSaveDir, initialValue)
{
    EXPECT_EQ(get_documents_dir(), g_save_dir);
}

TEST(TestOpenSaveFile, filename)
{
    constexpr std::string_view name{"tmp.txt"};
    if (fs::exists(name))
    {
        fs::remove(name);
    }
    const auto path{fs::path{ID_TEST_DATA_DIR} / name};
    if (exists(path))
    {
        fs::remove(path);
    }
    ValueSaver save_dir{g_save_dir, ID_TEST_DATA_DIR};

    std::FILE *file{open_save_file(name.data(), "w")};

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
    if (exists(path))
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
