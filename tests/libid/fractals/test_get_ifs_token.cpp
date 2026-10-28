// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <fractals/ifs.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <ostream>

namespace fs = std::filesystem;

using namespace id::fractals;
using namespace id::test::data;

namespace id::test
{

class TestGetIFSToken : public testing::Test
{
public:
    ~TestGetIFSToken() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    void skip_token();

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

void TestGetIFSToken::skip_token()
{
    get_next_ifs_token(m_buff.data(), m_file);
}

TEST_F(TestGetIFSToken, firstTokenIsName)
{
    const char *result = get_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(ID_TEST_FIRST_IFS_NAME, result);
}

struct IFSTokenPosition
{
    const char *name;
    int skipped_next;
    const char *expected;
};

void PrintTo(const IFSTokenPosition &input, std::ostream *out)
{
    *out << input.name;
}

class TestNextIFSToken : public TestGetIFSToken, public testing::WithParamInterface<IFSTokenPosition>
{
protected:
    void SetUp() override;
};

void TestNextIFSToken::SetUp()
{
    TestGetIFSToken::SetUp();
    get_ifs_token(m_buff.data(), m_file);
    for (int i = 0; i < GetParam().skipped_next; ++i)
    {
        skip_token();
    }
}

TEST_P(TestNextIFSToken, readsTokenAtStreamPosition)
{
    const char *result{get_next_ifs_token(m_buff.data(), m_file)};

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ(GetParam().expected, result);
}

constexpr int FIRST_IFS_PARAMETERS{7};
constexpr int FIRST_IFS_BODY_TOKENS{FIRST_IFS_PARAMETERS + 2};

INSTANTIATE_TEST_SUITE_P(Positions, TestNextIFSToken,
    testing::Values(IFSTokenPosition{"OpenBrace", 0, "{"},
        IFSTokenPosition{"FirstParameter", 1, ID_TEST_FIRST_IFS_PARAM1},
        IFSTokenPosition{"CloseBraceAfterOneRow", FIRST_IFS_PARAMETERS + 1, "}"},
        IFSTokenPosition{"SecondDefinitionName", FIRST_IFS_BODY_TOKENS, ID_TEST_SECOND_IFS_NAME},
        IFSTokenPosition{"SecondDefinitionQualifier", FIRST_IFS_BODY_TOKENS + 1, "(3D)"}),
    [](const testing::TestParamInfo<IFSTokenPosition> &info) { return info.param.name; });

} // namespace id::test
