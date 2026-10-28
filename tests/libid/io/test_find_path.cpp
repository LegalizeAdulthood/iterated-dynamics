// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/find_path.h>

#include "test_data.h"

#include <io/CurrentPathSaver.h>
#include <io/library.h>
#include <io/search_path.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

using namespace id::io;
using namespace id::test::data;

namespace id::test
{

using GetEnv = std::function<const char *(const char *)>;

class TestFindPath : public testing::Test
{
public:
    ~TestFindPath() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    GetEnv m_empty_env{[](const char *) -> const char * { return nullptr; }};
    std::string m_path{std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR};
    GetEnv m_env_path{[&](const char *var) { return std::string{var} == "PATH" ? m_path.c_str() : nullptr; }};

private:
    id::misc::ValueSaver<bool> m_check_current_directory{g_check_cur_dir};
};

void TestFindPath::SetUp()
{
    Test::SetUp();
    g_check_cur_dir = false;
    clear_read_library_path();
}

void TestFindPath::TearDown()
{
    clear_read_library_path();
    Test::TearDown();
}

struct PathSearchInput
{
    const char *name;
    fs::path requested;
    fs::path expected;
};

void PrintTo(const PathSearchInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestFindPathCurrentDirectory : public TestFindPath, public testing::WithParamInterface<PathSearchInput>
{
protected:
    void SetUp() override;
    void TearDown() override;

private:
    std::unique_ptr<CurrentPathSaver> m_current_directory;
};

void TestFindPathCurrentDirectory::SetUp()
{
    TestFindPath::SetUp();
    m_current_directory = std::make_unique<CurrentPathSaver>(ID_TEST_DATA_DIR);
    g_check_cur_dir = true;
}

void TestFindPathCurrentDirectory::TearDown()
{
    m_current_directory.reset();
    TestFindPath::TearDown();
}

TEST_P(TestFindPathCurrentDirectory, findsRequestedFile)
{
    const std::string result{find_path(GetParam().requested.string().c_str(), m_empty_env)};

    ASSERT_EQ(fs::path{GetParam().expected}.make_preferred(), fs::path{result});
}

INSTANTIATE_TEST_SUITE_P(CurrentDirectoryInputs, TestFindPathCurrentDirectory,
    testing::Values(PathSearchInput{"Filename", ID_TEST_IFS_FILE, fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE},
        PathSearchInput{"RelativePath", fs::path{ID_TEST_DATA_SUBDIR}.filename() / ID_TEST_IFS_FILE,
            fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE},
        PathSearchInput{"Missing", "goink.goink", {}}),
    [](const testing::TestParamInfo<PathSearchInput> &info) { return info.param.name; });

class TestFindPathReadLibrary : public TestFindPath, public testing::WithParamInterface<PathSearchInput>
{
protected:
    void SetUp() override;
};

void TestFindPathReadLibrary::SetUp()
{
    TestFindPath::SetUp();
    add_read_library(ID_TEST_DATA_DIR);
}

TEST_P(TestFindPathReadLibrary, findsRequestedFile)
{
    const std::string result{find_path(GetParam().requested.string().c_str(), m_empty_env)};

    ASSERT_EQ(fs::path{GetParam().expected}.make_preferred(), fs::path{result});
}

INSTANTIATE_TEST_SUITE_P(ReadLibraryInputs, TestFindPathReadLibrary,
    testing::Values(PathSearchInput{"Found", ID_TEST_IFS_FILE, fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE},
        PathSearchInput{"Missing", "goink.goink", {}}),
    [](const testing::TestParamInfo<PathSearchInput> &info) { return info.param.name; });

class TestFindPathEnvironment : public TestFindPath, public testing::WithParamInterface<PathSearchInput>
{
};

TEST_P(TestFindPathEnvironment, findsRequestedFile)
{
    const std::string result{find_path(GetParam().requested.string().c_str(), m_env_path)};

    ASSERT_EQ(fs::path{GetParam().expected}.make_preferred(), fs::path{result});
}

INSTANTIATE_TEST_SUITE_P(EnvironmentInputs, TestFindPathEnvironment,
    testing::Values(PathSearchInput{"FirstDirectory", ID_TEST_IFS_FILE, fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE},
        PathSearchInput{"SecondDirectory", ID_TEST_IFS_FILE2, fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE2},
        PathSearchInput{"Missing", "goink.goink", {}}),
    [](const testing::TestParamInfo<PathSearchInput> &info) { return info.param.name; });

TEST_F(TestFindPath, absolutePath)
{
    const fs::path absoluteLocation{(fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE).make_preferred()};
    ASSERT_TRUE(fs::exists(absoluteLocation));

    const std::string result = find_path(absoluteLocation.string().c_str(), m_empty_env);

    ASSERT_EQ(absoluteLocation.string(), result);
}

} // namespace id::test
