#include <get_ifs_token.h>

#include "test_data.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

class TestGetIFSToken : public testing::Test
{
public:
    ~TestGetIFSToken() override = default;

protected:
    void SetUp() override;
    void TearDown() override;

    fs::path m_ifs_file{fs::path{ID_TEST_DATA_DIR} / ID_TEST_IFS_FILE};
    std::FILE *m_file{};
    std::array<char, 100> m_buff{};
};

void TestGetIFSToken::SetUp()
{
    Test::SetUp();
    m_file = fopen(m_ifs_file.string().c_str(), "rt");
}

void TestGetIFSToken::TearDown()
{
    fclose(m_file);
    Test::TearDown();
}

TEST_F(TestGetIFSToken, firstToken)
{
    const char *result = get_ifs_token(m_buff.data(), m_file);

    ASSERT_NE(nullptr, result);
    ASSERT_STREQ("binary", result);
}
