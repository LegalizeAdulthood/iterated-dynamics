#include <ui/file_item.h>

#include "test_data.h"

#include <gtest/gtest.h>

using namespace id::test::data;

TEST(TestFindFileItem, formula)
{
    std::FILE *init_file = nullptr;
    std::filesystem::path path{ID_TEST_FRM_DIR};
    path /= ID_TEST_FRM_FILE;

    const bool result{find_file_item(path, "Fractint", &init_file, ItemType::FORMULA)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, init_file);

    if (init_file != nullptr)
    {
        std::fclose(init_file);
    }
}

TEST(TestFindFileItem, ifs)
{
    std::FILE *init_file = nullptr;
    std::filesystem::path path{ID_TEST_HOME_DIR};
    path /= "ifs";
    path /= ID_TEST_IFS_FILE;

    const bool result{find_file_item(path, ID_TEST_FIRST_IFS_NAME, &init_file, ItemType::IFS)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, init_file);

    if (init_file != nullptr)
    {
        std::fclose(init_file);
    }
}

TEST(TestFindFileItem, lindenmayerSystem)
{
    std::FILE *init_file = nullptr;
    std::filesystem::path path{ID_TEST_HOME_DIR};
    path/= "lsystem";
    path /= ID_TEST_LSYSTEM_FILE;

    const bool result{find_file_item(path, "Koch1", &init_file, ItemType::L_SYSTEM)};

    EXPECT_FALSE(result);
    EXPECT_NE(nullptr, init_file);

    if (init_file != nullptr)
    {
        std::fclose(init_file);
    }
}
