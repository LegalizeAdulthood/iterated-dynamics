#include <search_path.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

TEST(TestSearchPath, pathVarNotDefined)
{
    const std::string result = search_path(ID_TEST_IFS_FILE, "PATH", [](const char *) { return nullptr; });

    ASSERT_TRUE(result.empty());
}

TEST(TestSearchPath, fileNotInPath)
{
    const std::string path{ID_TEST_DATA_DIR};

    const std::string result = search_path("goink.goink", "PATH", [&](const char *) { return path.c_str(); });

    ASSERT_TRUE(result.empty());
}

TEST(TestSearchPath, fileInSinglePath)
{
    const std::string path{ID_TEST_DATA_DIR};

    const std::string result = search_path(ID_TEST_IFS_FILE, "PATH", [&](const char *) { return path.c_str(); });

    ASSERT_EQ((fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred().string(), result);
}

TEST(TestSearchPath, fileNotInSinglePath)
{
    const std::string path{ID_TEST_DATA_DIR};

    const std::string result = search_path("goink.goink", "PATH", [&](const char *) { return path.c_str(); });

    ASSERT_TRUE(result.empty());
}

TEST(TestSearchPath, fileInMultiplePath)
{
    const std::string path{std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR};

    const std::string result = search_path(ID_TEST_IFS_FILE2, "PATH", [&](const char *) { return path.c_str(); });

    ASSERT_EQ((fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE2).make_preferred().string(), result);
}

TEST(TestSearchPath, fileNotInMultiplePath)
{
    const std::string path{std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR};

    const std::string result = search_path("goink.goink", "PATH", [&](const char *) { return path.c_str(); });

    ASSERT_TRUE(result.empty());
}
