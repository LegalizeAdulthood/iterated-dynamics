// SPDX-License-Identifier: GPL-3.0-only
//
#include <ifs.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace
{

class TestGetIFSToken : public testing::Test
{
public:
    ~TestGetIFSToken() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    void skipToken();
    void skipFirstIFSDefinition();

    fs::path m_ifs_file{fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE};
    std::FILE *m_file{};
    std::array<char, 100> m_buff{};
};

void TestGetIFSToken::SetUp()
{
    Test::SetUp();
    m_file = std::fopen(m_ifs_file.string().c_str(), "rt");
}

void TestGetIFSToken::TearDown()
{
    std::fclose(m_file);
    Test::TearDown();
}

void TestGetIFSToken::skipToken()
{
    get_next_ifs_token(m_buff.data(), m_file);
}

void TestGetIFSToken::skipFirstIFSDefinition()
{
    constexpr int num_first_tokens = 7 + 1 + 1; // {, params, }
    get_ifs_token(m_buff.data(), m_file);       // name
    for (int i = 0; i < num_first_tokens; ++i)
    {
        skipToken();
    }
}

} // namespace

TEST_F(TestGetIFSToken, firstTokenIsName)
{
    const char *result = get_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(ID_TEST_FIRST_IFS_NAME, result);
}

TEST_F(TestGetIFSToken, secondTokenIsOpenBrace)
{
    get_ifs_token(m_buff.data(), m_file);

    const char *result = get_next_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ("{", result);
}

TEST_F(TestGetIFSToken, thirdTokenIsFirstParam)
{
    get_ifs_token(m_buff.data(), m_file);
    skipToken();

    const char *result = get_next_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(ID_TEST_FIRST_IFS_PARAM1, result);
}

TEST_F(TestGetIFSToken, firstIFSDefinitionHasOneRow)
{
    constexpr int num_param_tokens = 7;
    constexpr int num_open_brace_tokens = 1;
    get_ifs_token(m_buff.data(), m_file); // name
    for (int i = 0; i < num_param_tokens + num_open_brace_tokens; ++i)
    {
        skipToken();
    }

    const char *result = get_next_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ("}", result);
}

TEST_F(TestGetIFSToken, secondIFSDefinitionName)
{
    skipFirstIFSDefinition();

    const char *result = get_next_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(ID_TEST_SECOND_IFS_NAME, result);
}

TEST_F(TestGetIFSToken, secondIFSDefinition3DQualifier)
{
    skipFirstIFSDefinition();
    skipToken();

    const char *result = get_next_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ("(3D)", result);
}
