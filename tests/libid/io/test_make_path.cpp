// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/make_path.h>

#include <gtest/gtest.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace id::io;

namespace id::test
{

class TestMakePath : public testing::Test
{
public:
    ~TestMakePath() override = default;

protected:
    std::string m_result{"foo.txt"};
};

TEST_F(TestMakePath, empty)
{
    m_result = make_path(nullptr, nullptr, nullptr, nullptr);

    EXPECT_TRUE(m_result.empty());
}

TEST_F(TestMakePath, drive)
{
    m_result = make_path("C:", nullptr, nullptr, nullptr);

    ASSERT_EQ("C:", m_result);
}

TEST_F(TestMakePath, directory)
{
    m_result = make_path(nullptr, "foo", nullptr, nullptr);

    ASSERT_EQ(fs::path{"foo/"}.make_preferred().string(), m_result);
}

TEST_F(TestMakePath, filenameNullDirectory)
{
    m_result = make_path(nullptr, nullptr, "foo", nullptr);

    ASSERT_EQ("foo", m_result);
}

TEST_F(TestMakePath, filenameEmptyDirectory)
{
    m_result = make_path(nullptr, "", "foo", nullptr);

    ASSERT_EQ("foo", m_result);
}

TEST_F(TestMakePath, extension)
{
    m_result = make_path(nullptr, nullptr, nullptr, ".gif");

    ASSERT_EQ(".gif", m_result);
}

TEST_F(TestMakePath, filenameExtension)
{
    m_result = make_path(nullptr, nullptr, "foo", ".gif");

    ASSERT_EQ("foo.gif", m_result);
}

TEST_F(TestMakePath, directoryFilenameExtension)
{
    m_result = make_path(nullptr, "tmp", "foo", ".gif");

    ASSERT_EQ(fs::path{"tmp/foo.gif"}.make_preferred().string(), m_result);
}

#ifdef WIN32
// Drive letters are only present on Windows
TEST_F(TestMakePath, driveDirectoryFilenameExtension)
{
    m_result = make_path("C:", "tmp", "foo", ".gif");

    ASSERT_EQ(fs::path{"C:tmp/foo.gif"}.make_preferred().string(), m_result);
}
#endif

TEST_F(TestMakePath, directoryWithTrailingSlash)
{
    m_result = make_path(nullptr, "tmp/", nullptr, nullptr);

    ASSERT_EQ(fs::path{"tmp/"}.make_preferred().string(), m_result);
}

TEST_F(TestMakePath, filenameWithDot)
{
    m_result = make_path(nullptr, nullptr, "1997.04.30-Ship_of_Indecision", ".par");

    ASSERT_EQ("1997.04.30-Ship_of_Indecision.par", m_result);
}

} // namespace id::test
