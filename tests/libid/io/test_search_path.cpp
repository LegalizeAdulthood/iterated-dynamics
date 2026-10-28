// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/search_path.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <ostream>
#include <string>

namespace fs = std::filesystem;

using namespace id::io;
using namespace id::test::data;

namespace id::test
{

struct SearchPathInput
{
    const char *name;
    std::optional<std::string> environment;
    const char *filename;
    fs::path expected;
};

void PrintTo(const SearchPathInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestSearchPath : public testing::TestWithParam<SearchPathInput>
{
protected:
    void SetUp() override;

    std::optional<std::string> m_environment;
    std::function<const char *(const char *)> m_get_env{
        [this](const char *) { return m_environment ? m_environment->c_str() : nullptr; }};
};

void TestSearchPath::SetUp()
{
    Test::SetUp();
    m_environment = GetParam().environment;
}

TEST_P(TestSearchPath, findsFileUsingEnvironmentPath)
{
    const std::string result{search_path(GetParam().filename, "PATH", m_get_env)};

    ASSERT_EQ(fs::path{GetParam().expected}.make_preferred().string(), result);
}

INSTANTIATE_TEST_SUITE_P(EnvironmentInputs, TestSearchPath,
    testing::Values(SearchPathInput{"Undefined", std::nullopt, ID_TEST_IFS_FILE, {}},
        SearchPathInput{"MissingSingleDirectory", ID_TEST_DATA_DIR, "goink.goink", {}},
        SearchPathInput{
            "FoundSingleDirectory", ID_TEST_DATA_DIR, ID_TEST_IFS_FILE, fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE},
        SearchPathInput{"FoundMultipleDirectories",
            std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR, ID_TEST_IFS_FILE2,
            fs::path{ID_TEST_DATA_SUBDIR} / ID_TEST_IFS_FILE2},
        SearchPathInput{"MissingMultipleDirectories",
            std::string{ID_TEST_DATA_DIR} + PATH_SEPARATOR + ID_TEST_DATA_SUBDIR, "goink.goink", {}}),
    [](const testing::TestParamInfo<SearchPathInput> &info) { return info.param.name; });

} // namespace id::test
